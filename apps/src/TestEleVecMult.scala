
import spatial.dsl._

@spatial object TestEleVecMult extends SpatialApp {
  def main(args: Array[String]): Unit = {
    type T = FixPt[TRUE, _32, _32]
    // Source producer insertion started.
    // Tensor x1_Tensor_L8
    val x1_Tensor_L8_DRAM = DRAM[T](I32(96))
    val x1_Tensor_L8_data = Array.tabulate[T](I32(96)){_ => 0.1.to[T]}
    setMem(x1_Tensor_L8_DRAM, x1_Tensor_L8_data)

    // Tensor x0_Tensor_L7
    val x0_Tensor_L7_DRAM = DRAM[T](I32(96))
    val x0_Tensor_L7_data = Array.tabulate[T](I32(96)){_ => 0.1.to[T]}
    setMem(x0_Tensor_L7_DRAM, x0_Tensor_L7_data)

    // Source producer insertion done.

    // Dst producer insertion started.
    val x2_Tensor_L9_DRAM = DRAM[T](I32(96))
    // Dst producer insertion done.

    // Scalar insertion started.
    // Scalar insertion done.

    // Accel datapath insertion started.
    Accel {
      Foreach (I32(96) by I32(16)) { i_e2 => // e0
        val e7_x0_Tensor_L7_SRAM = SRAM[T](I32(16))
        e7_x0_Tensor_L7_SRAM load x0_Tensor_L7_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
        val e5_x1_Tensor_L8_SRAM = SRAM[T](I32(16))
        e5_x1_Tensor_L8_SRAM load x1_Tensor_L8_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
        val e4_x2_Tensor_L9_SRAM = SRAM[T](I32(16))
        Foreach (I32(16) by I32(1)) { i_e3 => // e1
          val e11 = e7_x0_Tensor_L7_SRAM(i_e3)
          val e10 = e5_x1_Tensor_L8_SRAM(i_e3)
          val e13 = e11 * e10
          e4_x2_Tensor_L9_SRAM(i_e3) = e13
        }

        x2_Tensor_L9_DRAM(i_e2 :: i_e2 + I32(16) par I32(4)) store e4_x2_Tensor_L9_SRAM
      }

    }
    // Accel datapath insertion done.

    val x2_Tensor_L9_result = getMem(x2_Tensor_L9_DRAM)
    printArray(x2_Tensor_L9_result)


  }
}