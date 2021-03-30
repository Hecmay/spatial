
import spatial.dsl._

@spatial object TestMatvecMult extends SpatialApp {
  def main(args: Array[String]): Unit = {
    type T = FixPt[TRUE, _32, _32]
    // Source producer insertion started.
    // Tensor x21_Concat_L13_L14
    val x21_Concat_L13_L14_DRAM = DRAM[T](I32(32), I32(96))
    val x21_Concat_L13_L14_data = Matrix.tabulate[T](I32(32), I32(96)){(_, _) => 0.1.to[T]}
    setMem(x21_Concat_L13_L14_DRAM, x21_Concat_L13_L14_data)

    // Tensor x22_Concat_L10_L11
    val x22_Concat_L10_L11_DRAM = DRAM[T](I32(96))
    val x22_Concat_L10_L11_data = Array.tabulate[T](I32(96)){_ => 0.1.to[T]}
    setMem(x22_Concat_L10_L11_DRAM, x22_Concat_L10_L11_data)

    // Source producer insertion done.

    // Dst producer insertion started.
    val x6_Tensor_L18_DRAM = DRAM[T](I32(32))
    // Dst producer insertion done.

    // Accel datapath insertion started.
    Accel {
      Foreach (I32(32) by I32(16) par I32(2)) { i_e2 => // e0
        val e7_x6_Tensor_L18_SRAM = SRAM[T](I32(16))
        Foreach (I32(16) by I32(1), I32(96) by I32(32) par I32(2)) { (i_e3, i_e4) => // e1
          val e10_x22_Concat_L10_L11_SRAM = SRAM[T](I32(32))
          e10_x22_Concat_L10_L11_SRAM load x22_Concat_L10_L11_DRAM(i_e4 :: i_e4 + I32(32) par I32(4))
          val e8_x21_Concat_L13_L14_SRAM = SRAM[T](I32(1), I32(32))
          e8_x21_Concat_L13_L14_SRAM load x21_Concat_L13_L14_DRAM(i_e4 :: i_e4 + I32(1), i_e3 :: i_e3 + I32(32) par I32(4))
          val e16 = Reduce (0.to[T]) (I32(32) by I32(1) par I32(16)) { i_e5 => // e6
            val e14 = e10_x22_Concat_L10_L11_SRAM(i_e5)
            val e13 = e8_x21_Concat_L13_L14_SRAM(i_e3, i_e5)
            val e16 = e13 * e14
            e16
          } {_+_}.value

          val e17 = Reg[T](0.to[T])
          e17 := mux(i_e4 == I32(0), e16, e17.value + e16)
          if (i_e4 == I32(64)) { e7_x6_Tensor_L18_SRAM(i_e3) = e17 }
        }

        x6_Tensor_L18_DRAM(i_e2 :: i_e2 + I32(16) par I32(4)) store e7_x6_Tensor_L18_SRAM
      }

    }
    // Accel datapath insertion done.

    val x6_Tensor_L18_result = getMem(x6_Tensor_L18_DRAM)
    printArray(x6_Tensor_L18_result)

  }
}