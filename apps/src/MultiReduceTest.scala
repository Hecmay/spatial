import spatial.dsl._

// This is to bypass spatial's issue on limiting number of dims passed to Reduce.apply.
// In theory we can do as many dims as we want, but the actual implementation only allows us
// to do at most 4-dim reduction. Using the Seq apply call works.
@spatial object MultiReduceTest extends SpatialApp with SmallCommonParams {
  def main(args: Array[String]): Unit = {
    val out = ArgOut[I32]
    val r = I32(4)
    Accel {
      out := Reduce(Reg[I32]).apply(Seq(
        Counter[I32](zero, r, one, one),
        Counter[I32](zero, r, one, one),
        Counter[I32](zero, r, one, one),
        Counter[I32](zero, r, one, one)
      )) { idxList =>
        val i = idxList.head
        val j = idxList(1)
        val k = idxList(2)
        val l = idxList(3)
        i + j + k + l
      } {_+_}.value
    }

    println(getArg(out))
  }
}
