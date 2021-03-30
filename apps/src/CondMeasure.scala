import spatial.dsl._

@spatial object CondMeasure extends SpatialApp {
  def main(args: Array[String]): Unit = {
    // The purpose of this app is to measure the latency of having a cond access.
    //  I also wonder what the II would be in this case (increased by 1?)
    val ctrlArg = ArgIn[Int]
    setArg(ctrlArg, args(I32(0)).to[Int])
    val data = Array.tabulate(I32(32)) (_ => I32(2))
    val dram = DRAM[Int](I32(32))
    setMem(dram, data)
    val dramOut = DRAM[Int](I32(16))

    // This should work?
    Accel {
      val outMem = SRAM[Int](I32(16))
      val mem = SRAM[Int](I32(32))
      mem load dram
      Foreach (I32(0) until I32(16)) { i =>
        val ii = I32(2) * i
        val iii = I32(2) * i + I32(1)
        val c = mem(ii)
        val cc = mem(iii)
        outMem(i) = max(c, cc)
      }

      dramOut store outMem
    }
  }
}
