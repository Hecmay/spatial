import spatial.dsl.I32

trait Conv3DParams {
  val N = I32(4)
  val W = I32(16)
  val H = I32(16)
  val D = I32(4)
  val C = I32(16)
  val C_o = I32(16)
  val k_h = I32(3)
  val k_w = I32(3)
  val k_d = I32(3)
  val one = I32(1)
  val zero = I32(0)
  val _1: I32 = one
  val _0: I32 = zero
  val reduce_tile_size = I32(8)
  val foreach_tile_size = I32(8)
  val k_w_center: spatial.dsl.I32 = one
  val k_h_center: spatial.dsl.I32 = one
  val k_d_center: spatial.dsl.I32 = one
  val ldPar = I32(4)
  val stPar = I32(4)
  val _mp = 8
  val _ip = 4
  val ip = I32(_ip)
  val mp = I32(_mp)
}
