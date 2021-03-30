import spatial.dsl._

@spatial object LineBufferBuffedConvMaxPool
    extends SpatialApp
    with SmallCommonParams {
  def main(args: Array[String]): Unit = {
    // The idea here is that we exploit the most locality by having a 2-dim cache.

    val Input =
      Tensor4.tabulate(N, W, H, C)((_, _, _, _) => I32(1))
    // k_h, k_w, C, C_o
    val Filter =
      Tensor4.tabulate(C_o, C, k_h, k_w)((_, _, _, _) => I32(1))
    // N, H_o, W_o, C_o ->
    val bias =
      Tensor4.tabulate(N, W_o, H_o, C_o)((_, _, _, _) => I32(1))

    val maxpoolSize = I32(2)

    val in_dram = DRAM[I32](N, W, H, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w)
    val bias_dram = DRAM[I32](N, W_o, H_o, C_o)
    val out_dram = DRAM[I32](N, W_o, H_o, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)
    setMem(bias_dram, bias)

    Accel {
      val in_sram_list =
        List.tabulate(4)(_ => SRAM[I32](I32(1), W, maxpoolSize, reduce_tile_size))
      val out_sram = SRAM[I32](I32(1), W_mpool, H_mpool, foreach_tile_size)
      val filter_sram = SRAM[I32](foreach_tile_size, reduce_tile_size, k_h, k_w)

      Foreach(N by step, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
        Foreach(foreach_tile_size by step) { iC_o_o =>
          val iC_o = iC_o_foreach_tile + iC_o_o
          Foreach(H by step, C by reduce_tile_size) { (iH, iC_reduce_tile) =>
            // TODO: I've got two options here: 1. do a 2D line buffer with flattened inner-most C_i dimension. 2. do a 3D line buffer by implementing one myself.
            // Something is funky here. Gotta think of it in a 2D way.
            in_sram_list.zipWithIndex.foreach {
              case (m, i) =>
                m load in_dram(
                  iN :: iN + step,
                  base :: base + W,
                  I32(i) :: I32(i) + step,
                  iC_reduce_tile :: iC_reduce_tile + reduce_tile_size par ldPar)
            }

          }
        }
      }
    }

    val result = getTensor4(out_dram)
    printTensor4(result)
  }
}
