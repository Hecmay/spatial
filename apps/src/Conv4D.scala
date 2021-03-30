//import spatial.dsl._
//import scala.collection.mutable.ListBuffer
//
//// TODO: Is it possible to instantiate such a big SRAM on FPGA? These SRAMs are used for reduction.
//@spatial object Conv4D extends SpatialApp with CommonParams {
//  def main(args: Array[String]): Unit = {
//    def multiDimIdx2FlattenedIdx(shapes: List[I32],
//                                 accIndices: List[I32]): I32 = {
//      // accIndices should be from outer-most to inner-most.
//      var accum = I32(1)
//      val multipliers = new ListBuffer[I32]() // TODO: This can be implemented in Sigma
//      multipliers += accum
//      shapes.reverse.map { s =>
//        accum = accum * s
//        multipliers += accum
//      }
//
//      (multipliers.reverse zip shapes).map { case (a, b) => a * b }.reduce {
//        case (a, b) => a + b
//      }
//    }
//    val Input = Array.tabulate(N * W * H * D * A * C) { _ =>
//      one
//    }
//    val Filter = Array.tabulate(C_o * C * k_h * k_w * k_d * k_a) { _ =>
//      one
//    }
//    val in_dram = DRAM[I32](N * W * H * D * A * C)
//    val filter_dram = DRAM[I32](C_o * C * k_h * k_w * k_d)
//    val out_dram = DRAM[I32](N * W * H * D * C_o)
//
//    setMem(in_dram, Input)
//    setMem(filter_dram, Filter)
//    Accel {
//      val in_sram = SRAM[I32](one * W * H * D * A * reduce_tile_size)
//      val filter_sram =
//        SRAM[I32](foreach_tile_size * reduce_tile_size * k_h * k_w * k_d * k_a)
//      val out_sram = SRAM[I32](one * W * H * D * A * foreach_tile_size)
//      Foreach(N by one, C_o by foreach_tile_size) { (iN, iC_o_foreach_tile) =>
//        Foreach(foreach_tile_size by one) { iC_o_o =>
//          val iC_o = iC_o_foreach_tile + iC_o_o
//          Foreach(C by reduce_tile_size) { iC_reduce_tile =>
//            val in_blockSize = one * W * H * D * A
//            val in_start = iC_reduce_tile * in_blockSize
//            val in_end = in_start + in_blockSize * reduce_tile_size
//            in_sram load in_dram(in_start :: in_end par ldPar)
//
//            val filter_blockSize = k_w * k_h * k_d * k_a
//            val filter_start =
//              (iC_o_foreach_tile * foreach_tile_size * reduce_tile_size +
//                iC_reduce_tile * reduce_tile_size) * filter_blockSize
//            val filter_end =
//              filter_start + filter_blockSize * foreach_tile_size * reduce_tile_size
//
//            filter_sram load filter_dram(filter_start :: filter_end par ldPar)
//            val foreachCounters = Seq(
//              Counter(W, one, one, one),
//              Counter(H, one, one, one),
//              Counter(D, one, one, one),
//              Counter(A, one, one, one)
//            )
//            Foreach.apply(foreachCounters) { foreachCounterList =>
//              val iW = foreachCounterList.head
//              val iH = foreachCounterList(1)
//              val iD = foreachCounterList(2)
//              val iA = foreachCounterList(3)
//              Foreach(reduce_tile_size by one) { iCC =>
//                val iC = iC_reduce_tile + iCC
//                // I need to use the Seq interface to get Spatial behave correctly on
//                // a high-rank reduce.
//                val reduceCounters = Seq(
//                  Counter(k_w, one, one, one),
//                  Counter(k_h, one, one, one),
//                  Counter(k_d, one, one, one),
//                  Counter(k_a, one, one, ip)
//                )
//                val partialConvResult =
//                  Reduce(Reg[I32])(reduceCounters) { counterList =>
//                    val is_kW = counterList.head
//                    val is_kH = counterList(1)
//                    val is_kD = counterList(2)
//                    val is_kA = counterList(3)
//                    val i_kW = is_kW - k_w_center
//                    val i_kH = is_kH - k_h_center
//                    val i_kD = is_kD - k_d_center
//                    val i_kA = is_kA - k_a_center
//                    val pixelIdxW = i_kW + iW
//                    val pixelIdxH = i_kH + iH
//                    val pixelIdxD = i_kD + iD
//                    val pixelIdxA = i_kA + iA
//                    // TODO: This may not be correct.
//                    val isInvalid =
//                      i_kW < zero | i_kW > W | i_kH < zero | i_kH > H | i_kD < zero | i_kD > D | i_kA < zero | i_kA > A
//                    val candidatePixel = in_sram(
//                      multiDimIdx2FlattenedIdx(
//                        List(W, H, D, A, reduce_tile_size),
//                        List(pixelIdxW, pixelIdxH, pixelIdxD, pixelIdxA, iCC)))
//                    val pixel = mux(isInvalid, zero, candidatePixel)
//                    // Warning: Spatial may be able to bank this, but the access pattern on the DRAM side needs to be laid out properly!
//                    // acc: iC_o * foreach_tile_size + iC * reduce_tile_size + i_kW * k_w + i_kH * k_h + i_kD * k_d)
//                    val weight = filter_sram(
//                      multiDimIdx2FlattenedIdx(
//                        List(foreach_tile_size,
//                             reduce_tile_size,
//                             k_h,
//                             k_w,
//                             k_d,
//                             k_a),
//                        List(iC_o, iC, i_kW, i_kH, i_kD, i_kA)))
//
//                    pixel * weight
//                  } { _ + _ }.value
//
//                // This is really a MemReduce case. Gotta think of how to do this...
//                // TODO: this addressing really seems wrong to me...
//                val out_idx = iN * one + iW * W + iH * H + iD * D + iC_o
//                out_sram(out_idx) = partialConvResult + mux(
//                  iC == zero,
//                  zero,
//                  out_sram(out_idx)
//                )
//              }
//
//            }
//          }
//
//        }
//
//        // TODO: Init a tile here. But it really seems that I should only initiate a transfer
//        //  when the index is correct (don't wanna have duplicate stores)
//        if (iC_o_foreach_tile == C_o - foreach_tile_size) {
//          val blockSize = N * W * H * D
//          val start_dram = iN * blockSize
//          val end_dram = start_dram + foreach_tile_size
//          out_dram(start_dram :: end_dram par stPar) store out_sram
//        }
//
//      }
//
//    }
//
//    val result = getMem(out_dram)
//    printArray(result)
//  }
//}
