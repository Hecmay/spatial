import spatial.dsl._

@spatial object DynamicDRAMAlloc extends SpatialApp {
  def main(args: Array[String]): Unit = {
    val dramLen = args(0).to[I32]
    val tileLen = 16

    val data = Array.tabulate[Int](dramLen)(i => i.to[Int])
    val dramIn = DRAM[Int](dramLen)
    setMem(dramIn, data)

    val dramOut = DRAM[Int](dramLen)

    Accel {
      Foreach (dramLen by tileLen par 2) { i =>
        val tile = SRAM[Int](tileLen)
        val outTile = SRAM[Int](tileLen)
        val validLen = min(tileLen, dramLen - i)
        tile load dramIn(i :: i + validLen)

        Foreach (validLen by 1 par 2) { j =>
          outTile(j) = tile(j) + 2
        }

        dramOut(i :: i + validLen) store outTile
      }
    }

    val result = getMem(dramOut)
    printArray(result, "result = ")
  }
  
}
