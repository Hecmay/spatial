import spatial.dsl._

@spatial object CounterObjectTest extends SpatialApp {
  def main(args: Array[String]): Unit = {
    val _32 = I32(32)
    val _1 = I32(1)
    val _0 = I32(0)
    val testIn = DRAM[I32](_32, _32, _32, _32, _32)
    val testOut = DRAM[I32](_32, _32, _32, _32, _32)
    Accel {
      val testMem = SRAM[I32](_32, _32, _32, _32, _32)
      val testOutMem = SRAM[I32](_32, _32, _32, _32, _32)
      testMem load testIn
      val counterSeq = Seq.tabulate(5)(_ => Counter(_0, _32, _1, _1))
      Foreach(
        counterSeq
      ) {
        case List(i, j, k, m, n) =>
          testOutMem(i, j, k, m, n) = testMem(i, j, k, m, n) + _1
      }

      testOut store testOutMem
    }

    printArray(getMem(testOut))

  }
}
