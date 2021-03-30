import spatial.dsl.I32
import spatial.{dsl => d}
import argon.lang._

trait CommonParams {
  type T = d.FixPt[d.TRUE, d._16, d._16]
  val tileSize = d.I32(32)
  val dPar = d.I32(4)
  val step = d.I32(1)
  val one = I32(1)
  val zero = I32(0)
  val baseAddr = d.I32(0)
  val base: d.I32 = baseAddr
  val i32 = d.I32(32)
  val i96 = d.I32(96)
  val i1 = d.I32(1)
  val i0 = d.I32(0)
  val ip = d.I32(4)
  val ips = 4
  val N = I32(4) // Do I always need a dim for batching? It's gonna be always the outermost dim!
  val D = I32(4)
  val A = I32(4)
  val H = I32(16)
  val W = I32(16)
  val W_mpool = I32(16)
  val H_mpool = I32(16)
  val H_o = I32(63)
  val W_o = I32(63)
  val C = I32(16)
  val C_o = I32(16)
  val k_h = I32(3)
  val k_h_center = I32(1)
  val k_w = I32(3)
  val k_w_center = I32(1)
  val k_d = I32(3)
  val k_d_center = I32(1)
  val k_a = I32(3)
  val k_a_center = I32(1)
  val ldPar = I32(4)
  val stPar = I32(4)
  val reduce_tile_size = I32(16)
  val foreach_tile_size = I32(16)

}
