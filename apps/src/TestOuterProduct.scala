//
//import spatial.dsl._
//
//@spatial object TestOuterProduct extends SpatialApp {
//  def main(args: Array[String]): Unit = {
//    type T = FixPt[TRUE, _32, _32]
//    // Source producer insertion started.
//    // Tensor x0_Tensor_L14
//    val x0_Tensor_L14_DRAM = DRAM[T](I32(32))
//    val x0_Tensor_L14_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
//    setMem(x0_Tensor_L14_DRAM, x0_Tensor_L14_data)
//
//    // Tensor x1_Tensor_L15
//    val x1_Tensor_L15_DRAM = DRAM[T](I32(32))
//    val x1_Tensor_L15_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
//    setMem(x1_Tensor_L15_DRAM, x1_Tensor_L15_data)
//
//    // Source producer insertion done.
//
//    // Dst producer insertion started.
//    val x2_Tensor_L16_DRAM = DRAM[T](I32(32), I32(32))
//    // Dst producer insertion done.
//
//    // Accel datapath insertion started.
//    Accel {
//      Foreach (I32(32) by I32(16) par I32(2)) { i_e2 => // e0
//        val e7_x1_Tensor_L15_SRAM = SRAM[T](I32(16))
//        e7_x1_Tensor_L15_SRAM load x1_Tensor_L15_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
//        val e6_x2_Tensor_L16_SRAM = SRAM[T](I32(1), I32(16))
//        val e4_x0_Tensor_L14_SRAM = SRAM[T](I32(16))
//        e4_x0_Tensor_L14_SRAM load x0_Tensor_L14_DRAM(i_e2 :: i_e2 + I32(16) par I32(4))
//        Foreach (I32(16) by I32(1)) { i_e3 => // e1
//          val e11 = e7_x1_Tensor_L15_SRAM(i_e3)
//          val e10 = e4_x0_Tensor_L14_SRAM(i_e3)
//          val e13 = e10 * e11
//          e6_x2_Tensor_L16_SRAM(i_e3, i_e3) = e13
//        }
//
//        x2_Tensor_L16_DRAM(i_e2 :: i_e2 + I32(1) par I32(4)) store e6_x2_Tensor_L16_SRAM
//      }
//
//    }
//    // Accel datapath insertion done.
//
//    val x2_Tensor_L16_result = getMatrix(x2_Tensor_L16_DRAM)
//    printMatrix(x2_Tensor_L16_result)
//
//  }
//}