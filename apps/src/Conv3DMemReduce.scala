import spatial.dsl._

@spatial object Conv3DMemReduce extends SpatialApp with Conv3DParams {
  private val _C = Counter

  def main(args: Array[String]): Unit = {
    val Input = Tensor5.tabulate(N, W, H, D, C)((_, _, _, _, _) => _1)
    val Filter = Tensor5.tabulate(C_o, C, k_h, k_w, k_d)((_, _, _, _, _) => _1)

    val in_dram = DRAM[I32](N, W, H, D, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w, k_d)
    val out_dram = DRAM[I32](N, W, H, D, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)

    Accel {
      Foreach(N by _1, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
        val out_sram = SRAM[I32](_1, W, H, D, foreach_tile_size)
        // TOOD: Seems that I shall par ip here.
        MemReduce(out_sram)(C by reduce_tile_size) { iC_reduce_tile =>
          val accum_sram = SRAM[I32](_1, W, H, D, foreach_tile_size)
          val in_sram = SRAM[I32](_1, W, H, D, reduce_tile_size)
          val filter_sram =
            SRAM[I32](foreach_tile_size, reduce_tile_size, k_h, k_w, k_d)
          in_sram load in_dram(
            iN :: iN + _1,
            _0 :: W,
            _0 :: H,
            _0 :: D,
            iC_reduce_tile :: iC_reduce_tile + reduce_tile_size par ldPar)

          filter_sram load filter_dram(
            iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size,
            iC_reduce_tile :: iC_reduce_tile + reduce_tile_size,
            _0 :: k_w,
            _0 :: k_h,
            _0 :: k_d par ldPar
          )

          val foreachCounters = Seq(_C(_0, foreach_tile_size, _1, mp),
                                    _C(_0, W, _1, _1),
                                    _C(_0, H, _1, _1),
                                    _C(_0, D, _1, _1))
          val reduceCounters = Seq(_C(_0, k_w, _1, _1),
            _C(_0, k_h, _1, _1),
            _C(_0, k_d, _1, _1),
            _C(_0, reduce_tile_size, _1, ip))

          Foreach(foreachCounters) { case List(iC_o_o, iW, iH, iD) =>
            accum_sram(_0, iW, iH, iD, iC_o_o) =
              Reduce(Reg[I32])(reduceCounters) { case List(is_kW, is_kH, is_kD, i_C) =>
                val i_kW = is_kW - k_w_center + iW
                val i_kH = is_kH - k_h_center + iH
                val i_kD = is_kD - k_d_center + iD
                val isInvalid = i_kW < zero | i_kW > W | i_kH < zero | i_kH > H | i_kD < zero | i_kD > D
                val pixel =
                  mux(isInvalid, _0, in_sram(_0, i_kW, i_kH, i_kD, i_C))
                val weight =
                  mux(isInvalid, _0, filter_sram(iC_o_o, i_C, i_kW, i_kH, i_kD))
                pixel * weight
              } { _ + _ }
          }

          accum_sram
        } { _ + _ }

        out_dram(
          iN :: iN + _1,
          _0 :: W,
          _0 :: H,
          _0 :: D,
          iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size par stPar) store out_sram

      }
    }

    val result = getTensor5(out_dram)
    printTensor5(result)
  }
}
