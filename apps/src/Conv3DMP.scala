import spatial.dsl._

@spatial abstract class Conv3DMP(
    _N: scala.Int,
    _W: scala.Int,
    _H: scala.Int,
    _D: scala.Int,
    _C: scala.Int,
    _C_o: scala.Int,
    _k_h: scala.Int,
    _k_w: scala.Int,
    _k_d: scala.Int,
    _kernel_size: scala.Int,
    _foreach_tile_size: scala.Int,
    _reduce_tile_size: scala.Int,
    _k_w_center: scala.Int,
    _k_h_center: scala.Int,
    _k_d_center: scala.Int,
    _ip: scala.Int,
    _mp: scala.Int,
    _ldPar: scala.Int,
    _stPar: scala.Int
) extends SpatialTest {
  def main(args: Array[String]): Unit = {
    val N = I32(_N)
    val W = I32(_W)
    val H = I32(_H)
    val D = I32(_D)
    val C = I32(_C)
    val C_o = I32(_C_o)
    val k_h = I32(_k_h)
    val k_w = I32(_k_w)
    val k_d = I32(_k_d)
    val k_w_center = I32(_k_w_center)
    val k_h_center = I32(_k_h_center)
    val k_d_center = I32(_k_d_center)
    val foreach_tile_size = I32(_foreach_tile_size)
    val reduce_tile_size = I32(_reduce_tile_size)
    val mp = I32(_mp)
    val ip = I32(_ip)
    val ldPar = I32(_ldPar)
    val stPar = I32(_stPar)
    val _Ctr = Counter
    val foreach_tile_size_per_bank = foreach_tile_size / mp

    def getKernelSize(_m: scala.Int): I32 =
      I32(scala.math.floor((_m - 1 * (_kernel_size - 1) - 1) / 1 + 1).toInt)
    val _1 = I32(1)
    val _0 = I32(0)
    val Input = Tensor5.tabulate(N, W, H, D, C)((_, _, _, _, _) => _1)
    val Filter = Tensor5.tabulate(C_o, C, k_h, k_w, k_d)((_, _, _, _, _) => _1)

    val Wo = getKernelSize(_W)
    val Ho = getKernelSize(_H)
    val Do = getKernelSize(_D)

    val in_dram = DRAM[I32](N, W, H, D, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w, k_d)
    val out_dram = DRAM[I32](N, Wo, Ho, Do, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)

    Accel {
      Foreach(N by _1, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
        val out_sram = SRAM[I32](_1, Wo, Ho, Do, foreach_tile_size)

        MemReduce(out_sram)(C by reduce_tile_size) { iC_reduce_tile =>
          val accum_sram = SRAM[I32](_1, Wo, Ho, Do, foreach_tile_size).buffer
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
                    Seq(_Ctr(_0, foreach_tile_size_per_bank, _1, _1),
                        _Ctr(k_w_center, W - k_w_center, _1, _1),
                        _Ctr(k_h_center, H - k_h_center, _1, _1),
                        _Ctr(k_d_center, D - k_d_center, _1, _1))

                  Foreach(foreachCounters) {
                    case List(iC_o_o, iW, iH, iD) =>
                      val reduceCounters =
                        Seq(_Ctr(_0, k_w, _1, _1),
                            _Ctr(_0, k_h, _1, _1),
                            _Ctr(_0, k_d, _1, _1),
                            _Ctr(_0, reduce_tile_size, _1, ip))
                      val iC_o_o_addr = iC_o_o + offset
                      val _v = Reduce(Reg[I32])(reduceCounters) {
                        case List(is_kW, is_kH, is_kD, i_C) =>
                          val i_kW = is_kW - k_w_center + iW
                          val i_kH = is_kH - k_h_center + iH
                          val i_kD = is_kD - k_d_center + iD
                          val pixel = in_sram(_0, i_kW, i_kH, i_kD, i_C)
                          val weight = m(iC_o_o, i_C, is_kW, is_kH, is_kD)
                          pixel * weight
                      } {
                        _ + _
                      }

                      accum_sram(_0,
                                 iW - k_w_center,
                                 iH - k_h_center,
                                 iD - k_d_center,
                                 iC_o_o_addr) = _v
                  }

                }
            }
          }

          accum_sram
        } { _ + _ }

        out_dram(
          iN :: iN + _1,
          _0 :: Wo,
          _0 :: Ho,
          _0 :: Do,
          iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size par stPar) store out_sram

      }
    }

    val result = getTensor5(out_dram)
    printTensor5(result)
  }
}

/**
  * Functional sim (to verify the correctness of implementation) params:
  * N = 4, W, H = 4, D = 4, iC = 4, oC = 2, kernel_size = 3
  */
class Conv3DMPCorrectnessCheck
    extends Conv3DMP(
      _N = 4,
      _W = 4,
      _H = 4,
      _D = 4,
      _C = 4,
      _C_o = 8,
      _k_h = 3,
      _k_w = 3,
      _k_d = 3,
      _kernel_size = 3,
      _foreach_tile_size = 2,
      _reduce_tile_size = 2,
      _k_w_center = 1,
      _k_h_center = 1,
      _k_d_center = 1,
      _ip = 2,
      _mp = 2,
      _ldPar = 1,
      _stPar = 1
    )

/**
  * GPU Params:
  *  N = 32, W, H = 64, D = 16, iC = 32, oC = 64
  */
class Conv3DMPChisel
    extends Conv3DMP(
      _N = 4,
      _W = 16,
      _H = 16,
      _D = 4,
      _C = 16,
      _C_o = 16,
      _k_h = 3,
      _k_w = 3,
      _k_d = 3,
      _kernel_size = 3,
      _foreach_tile_size = 16,
      _reduce_tile_size = 16,
      _k_w_center = 1,
      _k_h_center = 1,
      _k_d_center = 1,
      _ip = 16, // was 8
      _mp = 16, // was 8
      _ldPar = 4,
      _stPar = 8
    )
