import spatial.dsl._

@spatial object TestCeil extends SpatialTest {
  override def runtimeArgs: Args = "32"

  def main(args: Array[String]): Unit = {
    val a = args(0).to[Int]
    val b = ceil(a)

    val c = ArgIn[Int]
    val d = ArgOut[Int]
    setArg(c, b)
    Accel {
      d := c.value + 1.to[Int]
    }

    val result = getArg(d)
    println("result = " + result)
  }
}
