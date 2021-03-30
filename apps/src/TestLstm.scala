
import spatial.dsl._

@spatial object TestLstm extends SpatialApp {
  def main(args: Array[String]): Unit = {
    type T = FixPt[TRUE, _32, _32]
    // Source producer insertion started.
    // Tensor x112_Concat_L16_L17
    val x112_Concat_L16_L17_DRAM = DRAM[T](I32(96), I32(32))
    val x112_Concat_L16_L17_data = Matrix.tabulate[T](I32(96), I32(32)){(_, _) => 0.1.to[T]}
    setMem(x112_Concat_L16_L17_DRAM, x112_Concat_L16_L17_data)

    // Tensor x113_Concat_L20_L21
    val x113_Concat_L20_L21_DRAM = DRAM[T](I32(96), I32(32))
    val x113_Concat_L20_L21_data = Matrix.tabulate[T](I32(96), I32(32)){(_, _) => 0.1.to[T]}
    setMem(x113_Concat_L20_L21_DRAM, x113_Concat_L20_L21_data)

    // Tensor x5_Tensor_L14
    val x5_Tensor_L14_DRAM = DRAM[T](I32(32))
    val x5_Tensor_L14_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x5_Tensor_L14_DRAM, x5_Tensor_L14_data)

    // Tensor x110_Concat_L12_L13
    val x110_Concat_L12_L13_DRAM = DRAM[T](I32(32), I32(96))
    val x110_Concat_L12_L13_data = Matrix.tabulate[T](I32(32), I32(96)){(_, _) => 0.1.to[T]}
    setMem(x110_Concat_L12_L13_DRAM, x110_Concat_L12_L13_data)

    // Tensor x14_Tensor_L26
    val x14_Tensor_L26_DRAM = DRAM[T](I32(32))
    val x14_Tensor_L26_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x14_Tensor_L26_DRAM, x14_Tensor_L26_data)

    // Tensor x2_Tensor_L10
    val x2_Tensor_L10_DRAM = DRAM[T](I32(32))
    val x2_Tensor_L10_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x2_Tensor_L10_DRAM, x2_Tensor_L10_data)

    // Tensor x11_Tensor_L22
    val x11_Tensor_L22_DRAM = DRAM[T](I32(32))
    val x11_Tensor_L22_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x11_Tensor_L22_DRAM, x11_Tensor_L22_data)

    // Tensor x111_Concat_L8_L9
    val x111_Concat_L8_L9_DRAM = DRAM[T](I32(96))
    val x111_Concat_L8_L9_data = Array.tabulate[T](I32(96)){_ => 0.1.to[T]}
    setMem(x111_Concat_L8_L9_DRAM, x111_Concat_L8_L9_data)

    // Tensor x114_Concat_L24_L25
    val x114_Concat_L24_L25_DRAM = DRAM[T](I32(96), I32(32))
    val x114_Concat_L24_L25_data = Matrix.tabulate[T](I32(96), I32(32)){(_, _) => 0.1.to[T]}
    setMem(x114_Concat_L24_L25_DRAM, x114_Concat_L24_L25_data)

    // Tensor x8_Tensor_L18
    val x8_Tensor_L18_DRAM = DRAM[T](I32(32))
    val x8_Tensor_L18_data = Array.tabulate[T](I32(32)){_ => 0.1.to[T]}
    setMem(x8_Tensor_L18_DRAM, x8_Tensor_L18_data)

    // Source producer insertion done.

    // Dst producer insertion started.
    val x96_Tensor_L53_DRAM = DRAM[T](I32(32))
    val x95_Tensor_L52_DRAM = DRAM[T](I32(32))
    // Dst producer insertion done.

    // Scalar insertion started.
    // Scalar insertion done.

    def sigmoid(x: T): T = {
      val lutData = Seq.tabulate[T](128)(i => (1.0 / (1.0 + scala.math.exp(-i * 4.0 / 128.0))).to[T])
      val lut = LUT[T](lutData.length)(lutData: _*)
      val absX = abs(x)
      val lutIdx = (128.to[T] * absX / 4.0.to[T]).to[I32]
      val lutOut = lut(lutIdx)
      val funcOffsetVal = mux(x < 0.to[T], 1.0.to[T] - lutOut, lutOut)
      mux(x < -4.0.to[T], 0.to[T], mux(x > 4.0.to[T], 1.to[T], funcOffsetVal))
    }

    def tanh(x: T): T = {
      val lutData = Seq.tabulate[T](128)(i => scala.math.tanh(i * 4.0 / 128.0).to[T])
      val lut = LUT[T](lutData.length)(lutData: _*)
      val absX = abs(x)
      val lutIdx = (128.to[T] * absX / 4.0.to[T]).to[I32]
      val lutOut = lut(lutIdx)
      val funcOffsetVal = mux(x < 0.to[T], -lutOut, lutOut)
      mux(x < -4.0.to[T], -1.to[T], mux(x > 4.0.to[T], 1.to[T], funcOffsetVal))
    }

    // Accel datapath insertion started.
    Accel {
      Foreach (I32(32) by I32(32)) { i_e2 => // e0
        val e22_x8_Tensor_L18_SRAM = SRAM[T](I32(32))
        e22_x8_Tensor_L18_SRAM load x8_Tensor_L18_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e20_x14_Tensor_L26_SRAM = SRAM[T](I32(32))
        e20_x14_Tensor_L26_SRAM load x14_Tensor_L26_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e18_x2_Tensor_L10_SRAM = SRAM[T](I32(32))
        e18_x2_Tensor_L10_SRAM load x2_Tensor_L10_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e16_x11_Tensor_L22_SRAM = SRAM[T](I32(32))
        e16_x11_Tensor_L22_SRAM load x11_Tensor_L22_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e14_x5_Tensor_L14_SRAM = SRAM[T](I32(32))
        e14_x5_Tensor_L14_SRAM load x5_Tensor_L14_DRAM(i_e2 :: i_e2 + I32(32) par I32(4))
        val e13_x96_Tensor_L53_SRAM = SRAM[T](I32(32))
        val e70_x95_Tensor_L52_SRAM = SRAM[T](I32(32))
        Foreach (I32(32) by I32(1), I32(128) by I32(16)) { (i_e3, i_e4) => // e1
          val e39 = e22_x8_Tensor_L18_SRAM(i_e3)
          val e38 = e18_x2_Tensor_L10_SRAM(i_e3)
          val e37 = e16_x11_Tensor_L22_SRAM(i_e3)
          val e36 = e14_x5_Tensor_L14_SRAM(i_e3)
          val e35 = e20_x14_Tensor_L26_SRAM(i_e3)
          val e32_x114_Concat_L24_L25_SRAM = SRAM[T](I32(1), I32(16))
          e32_x114_Concat_L24_L25_SRAM load x114_Concat_L24_L25_DRAM(i_e3 :: i_e3 + I32(1), i_e4 :: i_e4 + I32(16) par I32(4))
          val e30_x112_Concat_L16_L17_SRAM = SRAM[T](I32(1), I32(16))
          e30_x112_Concat_L16_L17_SRAM load x112_Concat_L16_L17_DRAM(i_e3 :: i_e3 + I32(1), i_e4 :: i_e4 + I32(16) par I32(4))
          val e28_x113_Concat_L20_L21_SRAM = SRAM[T](I32(1), I32(16))
          e28_x113_Concat_L20_L21_SRAM load x113_Concat_L20_L21_DRAM(i_e3 :: i_e3 + I32(1), i_e4 :: i_e4 + I32(16) par I32(4))
          val e26_x110_Concat_L12_L13_SRAM = SRAM[T](I32(1), I32(16))
          e26_x110_Concat_L12_L13_SRAM load x110_Concat_L12_L13_DRAM(i_e3 :: i_e3 + I32(1), i_e4 :: i_e4 + I32(16) par I32(4))
          val e24_x111_Concat_L8_L9_SRAM = SRAM[T](I32(16))
          e24_x111_Concat_L8_L9_SRAM load x111_Concat_L8_L9_DRAM(i_e4 :: i_e4 + I32(16) par I32(4))
          val e63 = Reduce (0.to[T]) (I32(16) by I32(1) par I32(16)) { i_e5 => // e6
            val e41 = e24_x111_Concat_L8_L9_SRAM(i_e5)
            val e40 = e26_x110_Concat_L12_L13_SRAM(i_e3, i_e5)
            val e63 = e40 * e41
            e63
          } {_+_}.value

          val e66 = Reduce (0.to[T]) (I32(16) by I32(1) par I32(16)) { i_e7 => // e8
            val e43 = e24_x111_Concat_L8_L9_SRAM(i_e7)
            val e42 = e32_x114_Concat_L24_L25_SRAM(i_e3, i_e7)
            val e66 = e42 * e43
            e66
          } {_+_}.value

          val e51 = Reduce (0.to[T]) (I32(16) by I32(1) par I32(16)) { i_e9 => // e10
            val e45 = e24_x111_Concat_L8_L9_SRAM(i_e9)
            val e44 = e30_x112_Concat_L16_L17_SRAM(i_e3, i_e9)
            val e51 = e44 * e45
            e51
          } {_+_}.value

          val e50 = Reduce (0.to[T]) (I32(16) by I32(1) par I32(16)) { i_e11 => // e12
            val e47 = e24_x111_Concat_L8_L9_SRAM(i_e11)
            val e46 = e28_x113_Concat_L20_L21_SRAM(i_e3, i_e11)
            val e50 = e46 * e47
            e50
          } {_+_}.value

          val e60 = Reg[T](0.to[T])
          e60 := mux(i_e4 == I32(0), e63, e60.value + e63)
          val e53 = e60 + e36
          val e57 = sigmoid(e53)
          val e52 = e57 * e38
          val e69 = Reg[T](0.to[T])
          e69 := mux(i_e4 == I32(0), e50, e69.value + e50)
          val e61 = e69 + e37
          val e49 = sigmoid(e61)
          val e54 = Reg[T](0.to[T])
          e54 := mux(i_e4 == I32(0), e51, e54.value + e51)
          val e68 = e54 + e39
          val e58 = tanh(e68)
          val e62 = e49 * e58
          val e67 = e62 + e52
          if (i_e4 == I32(112)) { e70_x95_Tensor_L52_SRAM(i_e3) = e67 }
          val e56 = sigmoid(e67)
          val e64 = Reg[T](0.to[T])
          e64 := mux(i_e4 == I32(0), e66, e64.value + e66)
          val e59 = e64 + e35
          val e65 = sigmoid(e59)
          val e55 = e65 * e56
          if (i_e4 == I32(112)) { e13_x96_Tensor_L53_SRAM(i_e3) = e55 }
        }

        x96_Tensor_L53_DRAM(i_e2 :: i_e2 + I32(32) par I32(4)) store e13_x96_Tensor_L53_SRAM
        x95_Tensor_L52_DRAM(i_e2 :: i_e2 + I32(32) par I32(4)) store e70_x95_Tensor_L52_SRAM
      }

    }
    // Accel datapath insertion done.

    val x96_Tensor_L53_result = getMem(x96_Tensor_L53_DRAM)
    printArray(x96_Tensor_L53_result)
    val x95_Tensor_L52_result = getMem(x95_Tensor_L52_DRAM)
    printArray(x95_Tensor_L52_result)


  }
}