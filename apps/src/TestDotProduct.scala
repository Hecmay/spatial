
import spatial.dsl._

@spatial object TestDotProduct extends SpatialApp {
  def main(args: Array[String]): Unit = {
    type T = FixPt[TRUE, _32, _32]
    // Source producer insertion started.
    // Tensor x1_Tensor_L8
    val x1_Tensor_L8_DRAM = DRAM[T](I32(64))
    val x1_Tensor_L8_data = Array.tabulate[T](I32(64)){_ => 0.1.to[T]}
    setMem(x1_Tensor_L8_DRAM, x1_Tensor_L8_data)

    // Tensor x0_Tensor_L7
    val x0_Tensor_L7_DRAM = DRAM[T](I32(64))
    val x0_Tensor_L7_data = Array.tabulate[T](I32(64)){_ => 0.1.to[T]}
    setMem(x0_Tensor_L7_DRAM, x0_Tensor_L7_data)

    // Source producer insertion done.

    // Dst producer insertion started.
    // Dst producer insertion done.

    // Scalar insertion started.
    val x2_Scalar_L9_Reg = ArgOut[T]
    // Scalar insertion done.

    // Accel datapath insertion started.
    Accel {
      Foreach (I32(1) by I32(1)) { i_e5 => // e0
        val e15 = Reduce (0.to[T]) (I32(64) by I32(16)) { i_e2 => // e1
          val e8_x1_Tensor_L8_SRAM = SRAM[T](I32(16))
          e8_x1_Tensor_L8_SRAM load x1_Tensor_L8_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
          val e6_x0_Tensor_L7_SRAM = SRAM[T](I32(16))
          e6_x0_Tensor_L7_SRAM load x0_Tensor_L7_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
          val e15 = Reduce (0.to[T]) (I32(16) by I32(1) par I32(16)) { i_e3 => // e4
            val e11 = e8_x1_Tensor_L8_SRAM(i_e3)
            val e10 = e6_x0_Tensor_L7_SRAM(i_e3)
            val e15 = e10 * e11
            e15
          } {_+_}.value

          e15
        } {_+_}.value

        x2_Scalar_L9_Reg := e15
      }

    }
    // Accel datapath insertion done.


    val x2_Scalar_L9_result = getArg(x2_Scalar_L9_Reg)
    print(x2_Scalar_L9_result)

  }
}