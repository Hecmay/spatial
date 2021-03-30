import spatial.dsl._

object Conv3DFull
    extends Conv3DParamTuning(
      N = I32(32),
      W = I32(64),
      H = I32(64),
      D = I32(16),
      C = I32(32),
      C_o = I32(64),
      k_h = I32(3),
      k_w = I32(3),
      k_d = I32(3),
      k_h_center = I32(1),
      k_w_center = I32(1),
      k_d_center = I32(1),
      ldPar = I32(4),
      stPar = I32(8),
      reduce_tile_size = I32(16),
      foreach_tile_size = I32(16),
      mp = I32(8),
      ip = I32(8),
      _mp = 8
    )

@spatial abstract class Conv3DParamTuning(
    N: I32,
    W: I32,
    H: I32,
    D: I32,
    C: I32,
    C_o: I32,
    k_h: I32,
    k_w: I32,
    k_d: I32,
    k_h_center: I32,
    k_w_center: I32,
    k_d_center: I32,
    ldPar: I32,
    stPar: I32,
    reduce_tile_size: I32,
    foreach_tile_size: I32,
    mp: I32,
    ip: I32,
    _mp: scala.Int
) extends SpatialApp
    with Aliases {
  private val _C = Counter
  private val foreach_tile_size_per_bank = foreach_tile_size / mp

  def main(args: Array[String]): Unit = {
    scala.Console.println(s"ldPar = $ldPar, stPar = $stPar, mp = $mp, ip = $ip")
    val Input = Tensor5.tabulate(N, W, H, D, C)((_, _, _, _, _) => _1)
    val Filter = Tensor5.tabulate(C_o, C, k_h, k_w, k_d)((_, _, _, _, _) => _1)

    val in_dram = DRAM[I32](N, W, H, D, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w, k_d)
    val out_dram = DRAM[I32](N, W, H, D, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)

    // This test is to understand if I could manually unroll some indices to avoid banking effort.
    // About 1 min if mp is set to 1.
    scala.Console.println("ip = " + ip)
    Accel {
      Foreach(N by _1, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
        val out_sram = SRAM[I32](_1, W, H, D, foreach_tile_size)

        MemReduce(out_sram)(C by reduce_tile_size) { iC_reduce_tile =>
          val accum_sram = SRAM[I32](_1, W, H, D, foreach_tile_size).buffer
          val in_sram = SRAM[I32](_1, W, H, D, reduce_tile_size)
          in_sram load in_dram(
            iN :: iN + _1,
            _0 :: W,
            _0 :: H,
            _0 :: D,
            iC_reduce_tile :: iC_reduce_tile + reduce_tile_size par ldPar)

          val filter_sram_list = List.tabulate(_mp)(
            _ =>
              SRAM[I32](foreach_tile_size_per_bank,
                        reduce_tile_size,
                        k_h,
                        k_w,
                        k_d))
          Parallel {
            filter_sram_list.zipWithIndex.foreach {
              case (m, idx) =>
                scala.Console.println("Instantiating idx = " + idx)
                val offset = I32(idx) * foreach_tile_size_per_bank
                Pipe {
                  m load filter_dram(
                    iC_o_foreach_tile + offset :: iC_o_foreach_tile + offset + foreach_tile_size_per_bank,
                    iC_reduce_tile :: iC_reduce_tile + reduce_tile_size,
                    _0 :: k_w,
                    _0 :: k_h,
                    _0 :: k_d par ldPar
                  )
                  val foreachCounters =
                    Seq(_C(_0, foreach_tile_size_per_bank, _1, _1),
                        _C(_0, W, _1, _1),
                        _C(_0, H, _1, _1),
                        _C(_0, D, _1, _1))

                  Foreach(foreachCounters) {
                    case List(iC_o_o, iW, iH, iD) =>
                      val reduceCounters = Seq(_C(_0, k_w, _1, _1),
                                               _C(_0, k_h, _1, _1),
                                               _C(_0, k_d, _1, _1),
                                               _C(_0, reduce_tile_size, _1, ip))

                      accum_sram(_0, iW, iH, iD, iC_o_o + offset) =
                        Reduce(Reg[I32])(reduceCounters) {
                          case List(is_kW, is_kH, is_kD, i_C) =>
                            val i_kW = is_kW - k_w_center + iW
                            val i_kH = is_kH - k_h_center + iH
                            val i_kD = is_kD - k_d_center + iD
                            val isInvalid = i_kW < zero | i_kW > W | i_kH < zero | i_kH > H | i_kD < zero | i_kD > D
                            val pixel =
                              mux(isInvalid,
                                  _0,
                                  in_sram(_0, i_kW, i_kH, i_kD, i_C))
                            val weight =
                              mux(isInvalid,
                                  _0,
                                  m(iC_o_o, i_C, i_kW, i_kH, i_kD))
                            pixel * weight
                        } {
                          _ + _
                        }
                  }

                }
            }
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
