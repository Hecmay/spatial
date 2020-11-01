package spatial.codegen.pirgen

import argon._
import spatial.lang._
import spatial.node._
import spatial.metadata.memory._
import spatial.metadata.control._

trait PIRGenSparse extends PIRCodegen {

  override protected def genAccel(lhs: Sym[_], rhs: Op[_]): Unit = rhs match {
    case op@SparseSRAMNew(dims, autoBar)       => 
      stateMem(lhs, src"""SparseMem(stackTop[Ctrl].get.asInstanceOf[ControlTree], "SRAM").autoBar($autoBar)""")

    case op@SparseParSRAMNew(dims, par, autoBar, swizzle)       => 
      stateMem(lhs, src"""SparseMem(stackTop[Ctrl].get.asInstanceOf[ControlTree], "ParSRAM", $par).autoBar($autoBar).swizzle($swizzle)""")

    case op@SparseDRAMNew(dims, par, autoBar, seqLoad)       => 
      stateMem(lhs, src"""SparseMem(stackTop[Ctrl].get.asInstanceOf[ControlTree], "ParDRAM", $par).alias.update(${lhs.alias}).autoBar($autoBar).seqLoad($seqLoad)""")

    case op@SparseSRAMBankedRead(sram,bank,ofs,barriers,ens)       => 
      stateAccess(lhs, sram, ens) {
        src"SparseRead()" +
        src".addr(${assertOne(bank)})" +
        src".barriers($barriers)"
      }
    case op@SparseSRAMBankedRMWData(sram,data,bank,ofs,opcode,order,barriers,key,remoteAddr,ens)       => 
      stateAccess(lhs, sram, ens) {
        src"SparseRMWData($key)" +
        src".addr(${assertOne(bank)})" +
        src".input(${assertOne(data)})" +
        src".barriers($barriers)"
      }
    case op@SparseSRAMBankedWrite(sram,data,bank,ofs,barriers,ens) => 
      stateAccess(lhs, sram, ens, data=Some(data)) {
        src"SparseWrite()" +
        src".addr(${assertOne(bank)})" +
        src".barriers($barriers)"
      }
    case op@SparseSRAMBankedRMW(sram,data,bank,ofs,opcode,order,barriers,key,remoteAddr,ens) => 
      stateAccess(lhs, sram, ens) {
        src"""SparseRMW("$opcode","$order",$remoteAddr,$key)""" +
        src".addr(${assertOne(bank)})" + 
        src".input(${assertOne(data)})" +
        src".barriers($barriers)" 
      }
    //case op@LockDRAMBankedRead(dram,bank,ofs,lock,ens)       => 
      //stateAccess(lhs, dram, ens) {
        //src"LockRead()" +
        //src".addr(${assertOne(ofs)})" + 
        ////src".lock(${lock.map { lock => assertOne(lock) }})"
      //}
    //case op@LockDRAMBankedWrite(dram,data,bank,ofs,lock,ens) => 
      //stateAccess(lhs, dram, ens, data=Some(data)) {
        //src"LockWrite()" +
        //src".addr(${assertOne(ofs)})" + 
        ////src".lock(${lock.map { lock => assertOne(lock) }})"
      //}
    case op@BarrierNew(init, depth) => 
      state(lhs)(src"""Barrier(stackTop[Ctrl].get.asInstanceOf[ControlTree],$init,$depth).srcCtx("${lhs.ctx}").name("${lhs.name}")""")
    case op@BarrierPush(barrier) =>
      state(lhs, tp=Some("(Barrier, Boolean)"))(src"($barrier,true)")
    case op@BarrierPop(barrier) =>
      state(lhs, tp=Some("(Barrier, Boolean)"))(src"($barrier,false)")
    case _ => super.genAccel(lhs, rhs)
  }
}
