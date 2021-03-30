import spatial.dsl._

@spatial object SRAMBuffedConv2DMaxPool extends SpatialApp with SmallCommonParams {
  def main(args: Array[String]): Unit = {
    // N, H, W, C -> C (innermost), H, W, N (outermost)
    val Input =
      Tensor4.tabulate(N, W, H, C)((_, _, _, _) => I32(1))
    // k_h, k_w, C, C_o
    val Filter =
      Tensor4.tabulate(C_o, C, k_h, k_w)((_, _, _, _) => I32(1))
    // N, H_o, W_o, C_o ->
    val bias =
      Tensor4.tabulate(N, W_o, H_o, C_o)((_, _, _, _) => I32(1))

    val in_dram = DRAM[I32](N, W, H, C)
    val filter_dram = DRAM[I32](C_o, C, k_h, k_w)
    val bias_dram = DRAM[I32](N, W_o, H_o, C_o)
    val out_dram = DRAM[I32](N, W_o, H_o, C_o)

    setMem(in_dram, Input)
    setMem(filter_dram, Filter)
    setMem(bias_dram, bias)

    Accel {
      /**
        * TODO:
        *  I think this will be an interesting example for JAX, too (e.g., Conv4D!)
        *  The case for tiling the input dimensions:
        *  0. Best case: I can load the whole input tensor all at once.
        *  1. Second best case: I load an H x W * 1 batch every time. --> This is what I'm trying in this app!
        *  2. Kinda worse case: I load an n x W * 1 or 1 x W * 1 batch every time. Need a line buffer!
        *  3. Worst case ever: I load an n x m * 1 batch every time. Cannot even load a full line! What do I do now?
        *   - Should it be a failed case, i.e., just tell the user that I cannot do this?
        *  C, W, H, N
        *
        *  TODO:
        *    Another case we should think of is the DRAM Stream duplication. Let's say that we have
        *    a lot of kernels running in a pipeline fashion where intermediate data are stored off-chip.
        *    In this example, we cannot assign one dram stream per kernel. A more reasonable approach
        *    would be to assign 1 unified DRAM block and use addr gen to fetch weights and activations.
        */
      // In this example, we assume that we can at least fetch a whole image per channel per batch
      val in_sram = SRAM[I32](I32(1), W, H, reduce_tile_size)
      val buf_sram =
        SRAM[I32](I32(1), W, H, foreach_tile_size).buffer
      val bias_sram = SRAM[I32](I32(1), W, H, foreach_tile_size) // TODO: Fill in this SRAM!
      val out_sram = SRAM[I32](I32(1), W_mpool, H_mpool, foreach_tile_size)
      val filter_sram =
        SRAM[I32](foreach_tile_size, reduce_tile_size, k_h, k_w)

      Foreach(N by I32(1), C_o by foreach_tile_size) {
        (iN, iC_o_foreach_tile) =>
          Foreach(foreach_tile_size by I32(1)) { iC_o_o =>
            val iC_o = iC_o_foreach_tile + iC_o_o
            // Where should I put the convolution kernel?
            Foreach(C by reduce_tile_size) { iC_reduce_tile => // Is this a lowering opportunity?

              // I cannot fuse if I load in the following format!
              //  Reason: this format requires using an SRAM as a buffer.
              //  The buffer will be the intermediate memory that later gets consumed by maxpooling.
              in_sram load in_dram(
                iN :: iN + I32(1),
                I32(0) :: W,
                I32(0) :: H,
                iC_reduce_tile :: iC_reduce_tile + reduce_tile_size par ldPar)

              filter_sram load filter_dram(
                iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size,
                iC_reduce_tile :: iC_reduce_tile + reduce_tile_size,
                I32(0) :: k_w,
                I32(0) :: k_h par ldPar)

              // Inner-most reduce.
              Foreach(W by I32(1), H by I32(1)) { (iW, iH) =>
                Foreach(reduce_tile_size by I32(1)) { iCC => // TODO: I should be concerned about this II!
                  val iC = iC_reduce_tile + iCC
                  val partialConvResult =
                    Reduce(Reg[I32])(k_w by I32(1), k_h by I32(1) par ip) {
                      (is_kW, is_kH) =>
                        val i_kW = is_kW - k_w_center
                        val i_kH = is_kH - k_h_center
                        val pixelIdxW = i_kW + iW
                        val pixelIdxH = i_kH + iH
                        val isInValid = i_kW < I32(0) | i_kW > W | i_kH < I32(0) | i_kH > H
                        val pixel =
                          mux(isInValid,
                              I32(0),
                              in_sram(I32(0),
                                      pixelIdxW,
                                      pixelIdxH,
                                      iC_reduce_tile))
                        val weight = filter_sram(iC_o, iC, i_kW, i_kH)
                        pixel * weight
                    } { _ + _ }.value

                  buf_sram(iN, iW, iH, iC_o) = partialConvResult + mux(
                    iC == I32(0),
                    I32(0),
                    buf_sram(iN, iW, iH, iC_o))
                }
              }
            }

            // Bias-add + relu + maxpool
            bias_sram load bias_dram(
              iN :: iN + I32(1),
              I32(0) :: W_o,
              I32(0) :: H_o,
              iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size par ldPar)
            Foreach(W_mpool by I32(1), H_mpool by I32(1)) {
              (iW_mpool, iH_mpool) =>
                // TODO: How should we do about maxpooling?
                val iW = I32(2) * iW_mpool
                val iH = I32(2) * iH_mpool
                val pool_elements: List[I32] = List
                  .tabulate(2) { ii =>
                    List.tabulate(2) { jj =>
                      max(
                        I32(0),
                        buf_sram(iN, iW + I32(ii), iH + I32(jj), iC_o) + bias_sram(
                          iN,
                          iW + I32(ii),
                          iH + I32(jj),
                          iC_o)
                      )
                    }
                  }
                  .flatten
                val value = utils.math.ReduceTree(pool_elements: _*) {
                  max(_, _)
                }
                out_sram(iN, iW_mpool, iH_mpool, iC_o) = value
            }

            out_dram(
              iN :: iN + I32(1),
              I32(0) :: W_o,
              I32(0) :: H_o,
              iC_o_foreach_tile :: iC_o_foreach_tile + foreach_tile_size par stPar) store out_sram

          }
      }
      // TODO: How am I going to tile these things? Tiling a higher dim construct is much harder
      //  than an RNN / Transformer cell!
    }

    val result = getTensor4(out_dram)
    printTensor4(result)
  }
}
