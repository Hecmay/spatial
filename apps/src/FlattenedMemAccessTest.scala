import spatial.dsl._

@spatial object FlattenedMemAccessTest extends SpatialApp with Conv3DParams {
  private val _C = Counter

  def main(args: Array[String]): Unit = {
    // The goal of this test is to understand if flattening memory access can get banking work.
  }
}
