
import spatial.dsl._

@spatial object TestBiasAdd extends SpatialApp {
  def main(args: Array[String]): Unit = {
    type T = FixPt[TRUE, _32, _32]
    // Source producer insertion started.
    // Tensor x24_Concat_L11_L12
    val x24_Concat_L11_L12_DRAM = DRAM[T](I32(32), I32(96))
    val x24_Concat_L11_L12_data = Matrix.tabulate[T](I32(32), I32(96)){(_, _) => 0.1.to[T]}
    setMem(x24_Concat_L11_L12_DRAM, x24_Concat_L11_L12_data)

    // Tensor x25_Concat_L8_L9
    val x25_Concat_L8_L9_DRAM = DRAM[T](I32(96))
    val x25_Concat_L8_L9_data = Array.tabulate[T](I32(96)){_ => 0.1.to[T]}
    setMem(x25_Concat_L8_L9_DRAM, x25_Concat_L8_L9_data)

    // Tensor x4_Tensor_L13
    val x4_Tensor_L13_DRAM = DRAM[T](I32(32))
    val x4_Tensor_L13_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x4_Tensor_L13_DRAM, x4_Tensor_L13_data)

    // Source producer insertion done.

    // Dst producer insertion started.
    val x7_Tensor_L18_DRAM = DRAM[T](I32(32))
    // Dst producer insertion done.

    // Scalar insertion started.
    // Scalar insertion done.

    // Accel datapath insertion started.
    Accel {
      Foreach (I32(32) by I32(32) par I32(2)) { i_e2 => // e0
        val e8_x4_Tensor_L13_SRAM = SRAM[T](I32(32))
        e8_x4_Tensor_L13_SRAM load x4_Tensor_L13_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e7_x7_Tensor_L18_SRAM = SRAM[T](I32(32))
        Foreach (I32(32) by I32(1), I32(96) by I32(32)) { (i_e3, i_e4) => // e1
          val e15 = e8_x4_Tensor_L13_SRAM(i_e3)
          val e12_x25_Concat_L8_L9_SRAM = SRAM[T](I32(32))
          e12_x25_Concat_L8_L9_SRAM load x25_Concat_L8_L9_DRAM(i_e4 :: i_e4 + I32(32) par I32(4))
          val e10_x24_Concat_L11_L12_SRAM = SRAM[T](I32(1), I32(32))
          e10_x24_Concat_L11_L12_SRAM load x24_Concat_L11_L12_DRAM(i_e3 :: i_e3 + I32(1), i_e4 :: i_e4 + I32(32) par I32(4))
          val e21 = Reduce (0.to[T]) (I32(32) by I32(1) par I32(16)) { i_e5 => // e6
            val e17 = e12_x25_Concat_L8_L9_SRAM(i_e5)
            val e16 = e10_x24_Concat_L11_L12_SRAM(i_e3, i_e5)
            val e21 = e16 * e17
            e21
          } {_+_}.value

          val e19 = Reg[T](0.to[T])
          e19 := mux(i_e4 == I32(0), e21, e19.value + e21)
          val e20 = e19 + e15
          if (i_e4 == I32(64)) { e7_x7_Tensor_L18_SRAM(i_e3) = e20 }
        }

        x7_Tensor_L18_DRAM(i_e2 :: i_e2 + I32(32) par I32(4)) store e7_x7_Tensor_L18_SRAM
      }

    }
    // Accel datapath insertion done.

    val x7_Tensor_L18_result = getMem(x7_Tensor_L18_DRAM)
    printArray(x7_Tensor_L18_result)


  }
}