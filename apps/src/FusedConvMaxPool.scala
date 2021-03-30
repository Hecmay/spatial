import spatial.dsl._

@spatial object FusedConvMaxPool extends SpatialApp {
  def main(args: Array[String]): Unit = {
    val N = I32(32) // Do I always need a dim for batching? It's gonna be always the outermost dim!
    val H = I32(64)
    val W = I32(64)
    val W_mpool = I32(32)
    val H_mpool = I32(32)
    val H_o = I32(63)
    val W_o = I32(63)
    val C = I32(32)
    val C_o = I32(64)
    val k_h = I32(3)
    val k_h_center = I32(1)
    val k_w = I32(3)
    val k_w_center = I32(1)
    val ldPar = I32(4)
    val stPar = I32(4)
    val ip = I32(4)

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
      // Design:
      //      For (N by 1, C_o by fT)
      //	      For (fT by 1)
      //		      Reduce (C_i by rT)
      //			      For (W by 1, H by 1)
      //				      For (rT by 1)
      //					      Reduce (kW by 1, kH by 1)
      //		    For (W_mpool by 1, H_mpool by 1)
      //          pool()
      //=>
      //      For (N by 1, C_o by fT)
      //      	For (fT by 1)
      //      		For (W by 1, H by 1)
      //      			Reduce (C_i by rT)
      //      				For (rT by 1)
      //      					Reduce (kW by 1, kH by 1)
      //      		For (W_mpool by 1, H_mpool by 1)
      //            pool()
      // =>  
      //      For (N by 1, C_o by fT)
      //      	For (fT by 1)
      //      		For (W_mpool by 1, H_mpool by 1)
      //      			W = f(W_mpool)
      //      			H = f(H_mpool)
      //      			Reduce (C_i by rT)
      //      				For (rT by 1)
      //      					Reduce (kW by 1, kH by 1) — Issue: How am I gonna handle this? kW + W may not be covered by the image cache!
      //      			pool()
    }

    val result = getTensor4(out_dram)
    printTensor4(result)
  }
}
