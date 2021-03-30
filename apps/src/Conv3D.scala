import spatial.dsl._

@spatial object Conv3D extends SpatialApp with Conv3DParams {
  def main(args: Array[String]): Unit = {
    val Input = Tensor5.tabulate(N, W, H, D, C)((_, _, _, _, _) => one)
    val Filter = Tensor5.tabulate(C_o, C, k_h, k_w, k_d)((_, _, _, _, _) => one)
    val bias = Tensor5.tabulate(N, W, H, D, C)((_, _, _, _, _) => I32(1))

    val in_dram = DRAM[I32](N, W, H, D, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w, k_d)
    val out_dram = DRAM[I32](N, W, H, D, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)
    Accel {
      // TODO: What happens when I don't have enough SRAM for this kernel?
      val in_sram = SRAM[I32](one, W, H, D, reduce_tile_size)
      val filter_sram = SRAM[I32](foreach_tile_size, reduce_tile_size, k_h, k_w, k_d)
      val buf_sram = SRAM[I32](one, W, H, D, foreach_tile_size)
      val out_sram = SRAM[I32](one, W, H, D, foreach_tile_size)

      Foreach (N by one, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
        Foreach (C by reduce_tile_size) { iC_reduce_tile =>
          in_sram load in_dram(
            iN :: iN + one,
            zero :: W,
            zero :: H,
            zero :: D,
            iC_reduce_tile :: iC_reduce_tile + reduce_tile_size par ldPar
          )

          filter_sram load filter_dram(
            iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size,
            iC_reduce_tile :: iC_reduce_tile + reduce_tile_size,
            zero :: k_w,
            zero :: k_h,
            zero :: k_d par ldPar
          )

          Foreach (foreach_tile_size by one) { iC_o_o =>
            // TODO: This looks like a MemReduce? Would it work better if I were to replace this with a MemReduce?
            Foreach (W by one, H by one, D by one par mp) { (iW, iH, iD) =>
              val iC_o = iC_o_o + iC_o_foreach_tile
                Foreach (reduce_tile_size by one) { iCC =>
                  val iC = iC_reduce_tile + iCC
                  val partialConvResult =
                    Reduce(Reg[I32])(k_w by one, k_h by one, k_d by one par ip) { (is_kW, is_kH, is_kD) =>
                      val i_kW = is_kW - k_w_center
                      val i_kH = is_kH - k_h_center
                      val i_kD = is_kD - k_d_center
                      val pixelIdxW = i_kW + iW
                      val pixelIdxH = i_kH + iH
                      val pixelIdxD = i_kD + iD
                      val isInvalid = i_kW < zero | i_kW > W | i_kH < zero | i_kH > H | i_kD < zero | i_kD > D
                      val pixel = mux(isInvalid, zero, in_sram(zero, pixelIdxW, pixelIdxH, pixelIdxD, iCC))
                      val weight = filter_sram(iC_o, iC, i_kW, i_kH, i_kD)
                      pixel * weight
                    } {_+_}.value

                  out_sram(iN, iW, iH, iD, iC_o) = partialConvResult + mux(
                    iC == zero, zero, out_sram(iN, iW, iH, iD, iC_o)
                  )
                }
            }

          }

        }

        out_dram(
          iN :: iN + one,
          zero :: W,
          zero :: H,
          zero :: D,
          iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size par stPar
        ) store out_sram
      }
    }

    val result = getTensor5(out_dram)
    printTensor5(result)
  }
}
