import pir._
import pir.node._
import spade.param._
import prism.graph._

object AccelMain extends PIRApp {
  def staging(top:Top) = {
    import pirgenStaging._
    import top._
    top.name("OuterProduct_1")
    def split1 = {
    val x162 = save("x162", DRAM("vec1").dims(List(256)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:41:23").name("vec1")) // [DRAM] x162 = DRAMHostNew(List(Const(256)),Const(0))
    val x163 = save("x163", DRAM("vec2").dims(List(256)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:42:23").name("vec2")) // [DRAM] x163 = DRAMHostNew(List(Const(256)),Const(0))
    val x164 = save("x164", DRAM("out").dims(List(256, 256)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:43:22").name("out")) // [DRAM] x164 = DRAMHostNew(List(Const(256), Const(256)),Const(0))
    val x40 = save("x40", createCtrl(schedule=Sequenced)(UnitController()).sctx("OuterProduct.scala:48:11")) // [UnitController] x40 = AccelScope(Block(Const(())))
    val x167 = save("x167", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(64).tp(Fix(true, 32, 0))).max(Const(256).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:49:28")) // [Counter] x167 = CounterNew(Const(0),Const(256),Const(64),Const(1))
    val x168 = save("x168", List(x167).sctx("OuterProduct.scala:49:36")) // [List[Counter]] x168 = CounterChainNew(List(x167))
    val x289 = save("x289", createCtrl(schedule=Pipelined)(LoopController().cchain(x168)).sctx("OuterProduct.scala:49:36")) // [LoopController] x289 = UnrolledForeach(Set(),x168,Block(Const(())),List(List(b169)),List(List(b170)),None)
    val b169 = save("b169", CounterIter(List(0)).counter(x289.cchain.T(0)).resetParent(x289).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b169
    val b170 = save("b170", CounterValid(List(0)).counter(x289.cchain.T(0)).resetParent(x289).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b170
    val x171 = save("x171", SRAM().depth(2).dims(List(64)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:50:25").name("b1_0")) // [SRAM] x171 = SRAMNew(List(Const(64)),SRAM1[Fix[TRUE,_32,_0]])
    val x193 = save("x193", createCtrl(schedule=Streaming)(UnitController().en(Set(b170))).sctx("OuterProduct.scala:51:12")) // [UnitController] x193 = UnitPipe(Set(b170),Block(Const(())))
    }; split1
    def split2 = {
    val x172_offset = save("x172_offset", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 64, 0)).isInnerAccum(false).sctx("OuterProduct.scala:51:12").name("offset")) // [FIFO] x172_offset = StreamOutNew(BurstCmdBus)
    val x172_size = save("x172_size", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:51:12").name("size")) // [FIFO] x172_size = StreamOutNew(BurstCmdBus)
    val x172_isLoad = save("x172_isLoad", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Bool).isInnerAccum(false).sctx("OuterProduct.scala:51:12").name("isLoad")) // [FIFO] x172_isLoad = StreamOutNew(BurstCmdBus)
    streamOut(List(x172_offset, x172_size, x172_isLoad), DRAMBus)
    val x173 = save("x173", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:51:12")) // [FIFO] x173 = StreamInNew(BurstDataBus())
    streamIn(List(x173), DRAMBus)
    val x181 = save("x181", createCtrl(schedule=Sequenced)(UnitController()).sctx("OuterProduct.scala:51:12")) // [UnitController] x181 = UnitPipe(Set(),Block(Const(())))
    val x174 = save("x174", OpDef(op=FixSLA).addInput(lookup[CounterIter]("b169"),Const(2).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:51:12")) // [OpDef] x174 = FixSLA(b169,Const(2))
    val x175 = save("x175", OpDef(op=FixToFix).addInput(x174).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:51:12")) // [OpDef] x175 = FixToFix(x174,TRUE,_64,_0)
    val x176 = save("x176", dramAddress(lookup[DRAM]("x162")).name("x162_addr").tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:51:12")) // [MemRead] x176 = DRAMAddress(x162)
    val x177 = save("x177", OpDef(op=FixAdd).addInput(x175,x176).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:51:12")) // [OpDef] x177 = FixAdd(x175,x176)
    val x178_offset = save("x178_offset", x177) // [OpDef] x178_offset = SimpleStruct(ArrayBuffer((offset,x177), (size,Const(256)), (isLoad,Const(true))))
    val x178_size = save("x178_size", Const(256).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:51:12").name("size")) // [Const] x178_size = SimpleStruct(ArrayBuffer((offset,x177), (size,Const(256)), (isLoad,Const(true))))
    }; split2
    def split3 = {
    val x178_isLoad = save("x178_isLoad", Const(true).tp(Bool).sctx("OuterProduct.scala:51:12").name("isLoad")) // [Const] x178_isLoad = SimpleStruct(ArrayBuffer((offset,x177), (size,Const(256)), (isLoad,Const(true))))
    val x179 = save("x179", Const(true).tp(Bool).sctx("OuterProduct.scala:51:12")) // [Const] x179 = DRAMIsAlloc(x162)
    val x180_offset = save("x180_offset", MemWrite().setMem(lookup[FIFO]("x172_offset")).en(Set(x179)).data(lookup[OpDef]("x178_offset")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:51:12").name("offset")) // [MemWrite] x180_offset = StreamOutBankedWrite(x172,ArrayBuffer(x178),ArrayBuffer(Set(x179)))
    val x180_size = save("x180_size", MemWrite().setMem(lookup[FIFO]("x172_size")).en(Set(x179)).data(lookup[Const]("x178_size")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:51:12").name("size")) // [MemWrite] x180_size = StreamOutBankedWrite(x172,ArrayBuffer(x178),ArrayBuffer(Set(x179)))
    val x180_isLoad = save("x180_isLoad", MemWrite().setMem(lookup[FIFO]("x172_isLoad")).en(Set(x179)).data(x178_isLoad).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:51:12").name("isLoad")) // [MemWrite] x180_isLoad = StreamOutBankedWrite(x172,ArrayBuffer(x178),ArrayBuffer(Set(x179)))
    endState[Ctrl]
    val x182 = save("x182", FringeDenseLoad(lookup[DRAM]("x162")).offset(MemRead().setMem(lookup[FIFO]("x172_offset"))).size(MemRead().setMem(lookup[FIFO]("x172_size"))).data(MemWrite().setMem(lookup[FIFO]("x173")).data).sctx("OuterProduct.scala:51:12")) // [FringeDenseLoad] x182 = FringeDenseLoad(x162,x172,x173)
    val x183 = save("x183", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(64).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:51:12")) // [Counter] x183 = CounterNew(Const(0),Const(64),Const(1),Const(1))
    val x184 = save("x184", List(x183).sctx("OuterProduct.scala:51:12")) // [List[Counter]] x184 = CounterChainNew(List(x183))
    val x192 = save("x192", createCtrl(schedule=Pipelined)(LoopController().cchain(x184)).sctx("OuterProduct.scala:51:12")) // [LoopController] x192 = UnrolledForeach(Set(),x184,Block(Const(())),List(List(b185)),List(List(b186)),None)
    val b185 = save("b185", CounterIter(List(0)).counter(x192.cchain.T(0)).resetParent(x192).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b185
    val b186 = save("b186", CounterValid(List(0)).counter(x192.cchain.T(0)).resetParent(x192).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b186
    }; split3
    def split4 = {
    val x187 = save("x187", MemRead().setMem(lookup[FIFO]("x173")).en(Set(lookup[CounterValid]("b186"))).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:51:12")) // [MemRead] x187 = StreamInBankedRead(x173,ArrayBuffer(Set(b186)))
    val x188 = save("x188", x187) // [MemRead] x188 = VecApply(x187,0)
    val x191 = save("x191", BankedWrite().bank(List(Const(0).tp(Fix(true, 32, 0)))).offset(lookup[CounterIter]("b185")).setMem(lookup[SRAM]("x171")).en(Set(lookup[CounterValid]("b186"))).data(x188).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:51:12")) // [BankedWrite] x191 = SRAMBankedWrite(x171,Vector(x188),Vector(List(Const(0))),Vector(b185),Vector(Set(b186)))
    endState[Ctrl]
    endState[Ctrl]
    val x194 = save("x194", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(32).tp(Fix(true, 32, 0))).max(Const(256).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:52:30")) // [Counter] x194 = CounterNew(Const(0),Const(256),Const(32),Const(1))
    val x195 = save("x195", List(x194).sctx("OuterProduct.scala:52:39")) // [List[Counter]] x195 = CounterChainNew(List(x194))
    val x288 = save("x288", createCtrl(schedule=Pipelined)(LoopController().cchain(x195).en(Set(lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:52:39")) // [LoopController] x288 = UnrolledForeach(Set(b170),x195,Block(Const(())),List(List(b196)),List(List(b197)),None)
    val b196 = save("b196", CounterIter(List(0)).counter(x288.cchain.T(0)).resetParent(x288).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b196
    val b197 = save("b197", CounterValid(List(0)).counter(x288.cchain.T(0)).resetParent(x288).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b197
    val x198 = save("x198", SRAM().depth(2).dims(List(32)).darkVolume(0).banks(List(16)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:53:27").name("b2_0")) // [SRAM] x198 = SRAMNew(List(Const(32)),SRAM1[Fix[TRUE,_32,_0]])
    val x222 = save("x222", createCtrl(schedule=Streaming)(UnitController().en(Set(b197, lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:54:14")) // [UnitController] x222 = UnitPipe(Set(b197, b170),Block(Const(())))
    val x199_offset = save("x199_offset", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 64, 0)).isInnerAccum(false).sctx("OuterProduct.scala:54:14").name("offset")) // [FIFO] x199_offset = StreamOutNew(BurstCmdBus)
    }; split4
    def split5 = {
    val x199_size = save("x199_size", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:54:14").name("size")) // [FIFO] x199_size = StreamOutNew(BurstCmdBus)
    val x199_isLoad = save("x199_isLoad", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Bool).isInnerAccum(false).sctx("OuterProduct.scala:54:14").name("isLoad")) // [FIFO] x199_isLoad = StreamOutNew(BurstCmdBus)
    streamOut(List(lookup[FIFO]("x199_offset"), x199_size, x199_isLoad), DRAMBus)
    val x200 = save("x200", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:54:14")) // [FIFO] x200 = StreamInNew(BurstDataBus())
    streamIn(List(x200), DRAMBus)
    val x208 = save("x208", createCtrl(schedule=Sequenced)(UnitController()).sctx("OuterProduct.scala:54:14")) // [UnitController] x208 = UnitPipe(Set(),Block(Const(())))
    val x201 = save("x201", OpDef(op=FixSLA).addInput(lookup[CounterIter]("b196"),Const(2).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:54:14")) // [OpDef] x201 = FixSLA(b196,Const(2))
    val x202 = save("x202", OpDef(op=FixToFix).addInput(x201).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:54:14")) // [OpDef] x202 = FixToFix(x201,TRUE,_64,_0)
    val x203 = save("x203", dramAddress(lookup[DRAM]("x163")).name("x163_addr").tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:54:14")) // [MemRead] x203 = DRAMAddress(x163)
    val x204 = save("x204", OpDef(op=FixAdd).addInput(x202,x203).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:54:14")) // [OpDef] x204 = FixAdd(x202,x203)
    val x205_offset = save("x205_offset", x204) // [OpDef] x205_offset = SimpleStruct(ArrayBuffer((offset,x204), (size,Const(128)), (isLoad,Const(true))))
    val x205_size = save("x205_size", Const(128).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:54:14").name("size")) // [Const] x205_size = SimpleStruct(ArrayBuffer((offset,x204), (size,Const(128)), (isLoad,Const(true))))
    val x205_isLoad = save("x205_isLoad", Const(true).tp(Bool).sctx("OuterProduct.scala:54:14").name("isLoad")) // [Const] x205_isLoad = SimpleStruct(ArrayBuffer((offset,x204), (size,Const(128)), (isLoad,Const(true))))
    }; split5
    def split6 = {
    val x206 = save("x206", Const(true).tp(Bool).sctx("OuterProduct.scala:54:14")) // [Const] x206 = DRAMIsAlloc(x163)
    val x207_offset = save("x207_offset", MemWrite().setMem(lookup[FIFO]("x199_offset")).en(Set(x206)).data(lookup[OpDef]("x205_offset")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:54:14").name("offset")) // [MemWrite] x207_offset = StreamOutBankedWrite(x199,ArrayBuffer(x205),ArrayBuffer(Set(x206)))
    val x207_size = save("x207_size", MemWrite().setMem(lookup[FIFO]("x199_size")).en(Set(x206)).data(lookup[Const]("x205_size")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:54:14").name("size")) // [MemWrite] x207_size = StreamOutBankedWrite(x199,ArrayBuffer(x205),ArrayBuffer(Set(x206)))
    val x207_isLoad = save("x207_isLoad", MemWrite().setMem(lookup[FIFO]("x199_isLoad")).en(Set(x206)).data(lookup[Const]("x205_isLoad")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:54:14").name("isLoad")) // [MemWrite] x207_isLoad = StreamOutBankedWrite(x199,ArrayBuffer(x205),ArrayBuffer(Set(x206)))
    endState[Ctrl]
    val x209 = save("x209", FringeDenseLoad(lookup[DRAM]("x163")).offset(MemRead().setMem(lookup[FIFO]("x199_offset"))).size(MemRead().setMem(lookup[FIFO]("x199_size"))).data(MemWrite().setMem(lookup[FIFO]("x200")).data).sctx("OuterProduct.scala:54:14")) // [FringeDenseLoad] x209 = FringeDenseLoad(x163,x199,x200)
    val x210 = save("x210", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(32).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:54:14")) // [Counter] x210 = CounterNew(Const(0),Const(32),Const(1),Const(1))
    val x211 = save("x211", List(x210).sctx("OuterProduct.scala:54:14")) // [List[Counter]] x211 = CounterChainNew(List(x210))
    val x221 = save("x221", createCtrl(schedule=Pipelined)(LoopController().cchain(x211)).sctx("OuterProduct.scala:54:14")) // [LoopController] x221 = UnrolledForeach(Set(),x211,Block(Const(())),List(List(b212)),List(List(b213)),None)
    val b212 = save("b212", CounterIter(List(0)).counter(x221.cchain.T(0)).resetParent(x221).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b212
    val b213 = save("b213", CounterValid(List(0)).counter(x221.cchain.T(0)).resetParent(x221).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b213
    val x214 = save("x214", MemRead().setMem(lookup[FIFO]("x200")).en(Set(b213)).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:54:14")) // [MemRead] x214 = StreamInBankedRead(x200,ArrayBuffer(Set(b213)))
    }; split6
    def split7 = {
    val x215 = save("x215", lookup[MemRead]("x214")) // [MemRead] x215 = VecApply(x214,0)
    val x323 = save("x323", OpDef(op=FixAnd).addInput(lookup[CounterIter]("b212"),Const(15).tp(Fix(true, 32, 0))).tp(Fix(true, 32, 0)).sctx("RewriteTransformer.scala:46:12")) // [OpDef] x323 = FixAnd(b212,Const(15))
    val x217 = save("x217", OpDef(op=FixSRA).addInput(lookup[CounterIter]("b212"),Const(4).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:54:14")) // [OpDef] x217 = FixSRA(b212,Const(4))
    val x220 = save("x220", BankedWrite().bank(List(x323)).offset(x217).setMem(lookup[SRAM]("x198")).en(Set(lookup[CounterValid]("b213"))).data(x215).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:54:14")) // [BankedWrite] x220 = SRAMBankedWrite(x198,Vector(x215),Vector(List(x323)),Vector(x217),Vector(Set(b213)))
    endState[Ctrl]
    endState[Ctrl]
    val x223 = save("x223", SRAM().depth(2).dims(List(64, 32)).darkVolume(0).banks(List(16)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:55:32").name("outTile_0")) // [SRAM] x223 = SRAMNew(List(Const(64), Const(32)),SRAM2[Fix[TRUE,_32,_0]])
    val x224 = save("x224", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(64).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:57:23")) // [Counter] x224 = CounterNew(Const(0),Const(64),Const(1),Const(1))
    val x225 = save("x225", Counter(par=16).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(32).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:57:36")) // [Counter] x225 = CounterNew(Const(0),Const(32),Const(1),Const(16))
    val x226 = save("x226", List(x224, x225).sctx("OuterProduct.scala:57:44")) // [List[Counter]] x226 = CounterChainNew(List(x224, x225))
    val x248 = save("x248", createCtrl(schedule=Pipelined)(LoopController().cchain(x226).en(Set(lookup[CounterValid]("b197"), lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:57:44")) // [LoopController] x248 = UnrolledForeach(Set(b197, b170),x226,Block(Const(())),List(List(b227), List(b228)),List(List(b229), List(b230)),None)
    val b227 = save("b227", CounterIter(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).counter(x248.cchain.T(0)).resetParent(x248).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b227
    val b228 = save("b228", CounterIter(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).counter(x248.cchain.T(1)).resetParent(x248).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b228
    }; split7
    def split8 = {
    val b229 = save("b229", CounterValid(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).counter(lookup[LoopController]("x248").cchain.T(0)).resetParent(lookup[LoopController]("x248")).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b229
    val b230 = save("b230", CounterValid(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).counter(lookup[LoopController]("x248").cchain.T(1)).resetParent(lookup[LoopController]("x248")).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b230
    val x233 = save("x233", BankedRead().bank(List(Const(0).tp(Fix(true, 32, 0)))).offset(lookup[CounterIter]("b227")).setMem(lookup[SRAM]("x171")).en(Set(b229, b230, lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).port(Some(1)).muxPort(0).broadcast(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).castgroup(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:77")) // [BankedRead] x233 = SRAMBankedRead(x171,Vector(List(Const(0))),Vector(b227),Vector(Set(b229, b230, b197, b170)),Vec[Fix[TRUE,_32,_0]])
    val x234 = save("x234", x233) // [BankedRead] x234 = VecApply(x233,0)
    val x236 = save("x236", OpDef(op=FixSRA).addInput(lookup[CounterIter]("b228"),Const(4).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:86")) // [OpDef] x236 = FixSRA(b228,Const(4))
    val x239 = save("x239", BankedRead().bank(List(Const(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).tp(Fix(true, 32, 0)))).offset(x236).setMem(lookup[SRAM]("x198")).en(Set(b229, b230, lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).port(Some(1)).muxPort(0).broadcast(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).castgroup(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:86")) // [BankedRead] x239 = SRAMBankedRead(x198,Vector(List(b324)),Vector(x236),Vector(Set(b229, b230, b197, b170)),Vec[Fix[TRUE,_32,_0]])
    val x240 = save("x240", x239) // [BankedRead] x240 = VecApply(x239,0)
    val x241 = save("x241", OpDef(op=FixMul).addInput(x234,x240).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:82")) // [OpDef] x241 = FixMul(x234,x240)
    val x242 = save("x242", OpDef(op=FixSLA).addInput(lookup[CounterIter]("b227"),Const(1).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:73")) // [OpDef] x242 = FixSLA(b227,Const(1))
    val x243 = save("x243", OpDef(op=FixAdd).addInput(x242,x236).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:57:73")) // [OpDef] x243 = FixAdd(x242,x236)
    val x247 = save("x247", BankedWrite().bank(List(Const(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).tp(Fix(true, 32, 0)))).offset(x243).setMem(lookup[SRAM]("x223")).en(Set(b229, b230, lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).data(x241).port(Some(0)).muxPort(0).broadcast(List(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).castgroup(List(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)).sctx("OuterProduct.scala:57:73")) // [BankedWrite] x247 = SRAMBankedWrite(x223,Vector(x241),Vector(List(b324)),Vector(x243),Vector(Set(b229, b230, b197, b170)))
    }; split8
    def split9 = {
    endState[Ctrl]
    val x249 = save("x249", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(64).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:58:42")) // [Counter] x249 = CounterNew(Const(0),Const(64),Const(1),Const(1))
    val x250 = save("x250", List(x249).sctx("OuterProduct.scala:58:42")) // [List[Counter]] x250 = CounterChainNew(List(x249))
    val x287 = save("x287", createCtrl(schedule=Streaming)(LoopController().cchain(x250).en(Set(lookup[CounterValid]("b197"), lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:58:42")) // [LoopController] x287 = UnrolledForeach(Set(b197, b170),x250,Block(Const(())),List(List(b251)),List(List(b252)),None)
    val b251 = save("b251", CounterIter(List(0)).counter(x287.cchain.T(0)).resetParent(x287).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b251
    val b252 = save("b252", CounterValid(List(0)).counter(x287.cchain.T(0)).resetParent(x287).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b252
    val x253_offset = save("x253_offset", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 64, 0)).isInnerAccum(false).sctx("OuterProduct.scala:58:42").name("offset")) // [FIFO] x253_offset = StreamOutNew(BurstCmdBus)
    val x253_size = save("x253_size", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:58:42").name("size")) // [FIFO] x253_size = StreamOutNew(BurstCmdBus)
    val x253_isLoad = save("x253_isLoad", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Bool).isInnerAccum(false).sctx("OuterProduct.scala:58:42").name("isLoad")) // [FIFO] x253_isLoad = StreamOutNew(BurstCmdBus)
    streamOut(List(x253_offset, x253_size, x253_isLoad), DRAMBus)
    val x254__1 = save("x254__1", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Fix(true, 32, 0)).isInnerAccum(false).sctx("OuterProduct.scala:58:42").name("_1")) // [FIFO] x254__1 = StreamOutNew(BurstFullDataBus())
    val x254__2 = save("x254__2", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Bool).isInnerAccum(false).sctx("OuterProduct.scala:58:42").name("_2")) // [FIFO] x254__2 = StreamOutNew(BurstFullDataBus())
    streamOut(List(x254__1, x254__2), DRAMBus)
    val x255 = save("x255", FIFO().depth(1).dims(List(1)).darkVolume(0).banks(List(1)).tp(Bool).isInnerAccum(false).sctx("OuterProduct.scala:58:42")) // [FIFO] x255 = StreamInNew(BurstAckBus)
    }; split9
    def split10 = {
    streamIn(List(lookup[FIFO]("x255")), DRAMBus)
    val x266 = save("x266", createCtrl(schedule=Sequenced)(UnitController().en(Set(lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:58:42")) // [UnitController] x266 = UnitPipe(Set(b252, b197, b170),Block(Const(())))
    val x256 = save("x256", OpDef(op=FixAdd).addInput(lookup[CounterIter]("b169"),lookup[CounterIter]("b251")).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x256 = FixAdd(b169,b251)
    val x257 = save("x257", OpDef(op=FixSLA).addInput(x256,Const(8).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x257 = FixSLA(x256,Const(8))
    val x258 = save("x258", OpDef(op=FixAdd).addInput(x257,lookup[CounterIter]("b196")).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x258 = FixAdd(x257,b196)
    val x259 = save("x259", OpDef(op=FixSLA).addInput(x258,Const(2).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x259 = FixSLA(x258,Const(2))
    val x260 = save("x260", OpDef(op=FixToFix).addInput(x259).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x260 = FixToFix(x259,TRUE,_64,_0)
    val x261 = save("x261", dramAddress(lookup[DRAM]("x164")).name("x164_addr").tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:58:42")) // [MemRead] x261 = DRAMAddress(x164)
    val x262 = save("x262", OpDef(op=FixAdd).addInput(x260,x261).tp(Fix(true, 64, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x262 = FixAdd(x260,x261)
    val x263_offset = save("x263_offset", x262) // [OpDef] x263_offset = SimpleStruct(ArrayBuffer((offset,x262), (size,Const(128)), (isLoad,Const(false))))
    val x263_size = save("x263_size", Const(128).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42").name("size")) // [Const] x263_size = SimpleStruct(ArrayBuffer((offset,x262), (size,Const(128)), (isLoad,Const(false))))
    val x263_isLoad = save("x263_isLoad", Const(false).tp(Bool).sctx("OuterProduct.scala:58:42").name("isLoad")) // [Const] x263_isLoad = SimpleStruct(ArrayBuffer((offset,x262), (size,Const(128)), (isLoad,Const(false))))
    }; split10
    def split11 = {
    val x264 = save("x264", Const(true).tp(Bool).sctx("OuterProduct.scala:58:42")) // [Const] x264 = DRAMIsAlloc(x164)
    val x265_offset = save("x265_offset", MemWrite().setMem(lookup[FIFO]("x253_offset")).en(Set(x264)).data(lookup[OpDef]("x263_offset")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:58:42").name("offset")) // [MemWrite] x265_offset = StreamOutBankedWrite(x253,ArrayBuffer(x263),ArrayBuffer(Set(x264)))
    val x265_size = save("x265_size", MemWrite().setMem(lookup[FIFO]("x253_size")).en(Set(x264)).data(lookup[Const]("x263_size")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:58:42").name("size")) // [MemWrite] x265_size = StreamOutBankedWrite(x253,ArrayBuffer(x263),ArrayBuffer(Set(x264)))
    val x265_isLoad = save("x265_isLoad", MemWrite().setMem(lookup[FIFO]("x253_isLoad")).en(Set(x264)).data(lookup[Const]("x263_isLoad")).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:58:42").name("isLoad")) // [MemWrite] x265_isLoad = StreamOutBankedWrite(x253,ArrayBuffer(x263),ArrayBuffer(Set(x264)))
    endState[Ctrl]
    val x267 = save("x267", Counter(par=1).min(Const(0).tp(Fix(true, 32, 0))).step(Const(1).tp(Fix(true, 32, 0))).max(Const(32).tp(Fix(true, 32, 0))).sctx("OuterProduct.scala:58:42")) // [Counter] x267 = CounterNew(Const(0),Const(32),Const(1),Const(1))
    val x268 = save("x268", List(x267).sctx("OuterProduct.scala:58:42")) // [List[Counter]] x268 = CounterChainNew(List(x267))
    val x282 = save("x282", createCtrl(schedule=Pipelined)(LoopController().cchain(x268).en(Set(lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:58:42")) // [LoopController] x282 = UnrolledForeach(Set(b252, b197, b170),x268,Block(Const(())),List(List(b269)),List(List(b270)),None)
    val b269 = save("b269", CounterIter(List(0)).counter(x282.cchain.T(0)).resetParent(x282).tp(Fix(true, 32, 0)).sctx("Staging.scala:26:82")) // [CounterIter] b269
    val b270 = save("b270", CounterValid(List(0)).counter(x282.cchain.T(0)).resetParent(x282).tp(Bool).sctx("Staging.scala:26:82")) // [CounterValid] b270
    val x325 = save("x325", OpDef(op=FixAnd).addInput(b269,Const(15).tp(Fix(true, 32, 0))).tp(Fix(true, 32, 0)).sctx("RewriteTransformer.scala:46:12")) // [OpDef] x325 = FixAnd(b269,Const(15))
    val x272 = save("x272", OpDef(op=FixSLA).addInput(lookup[CounterIter]("b251"),Const(1).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x272 = FixSLA(b251,Const(1))
    }; split11
    def split12 = {
    val x273 = save("x273", OpDef(op=FixSRA).addInput(lookup[CounterIter]("b269"),Const(4).tp(Fix(true, 16, 0))).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x273 = FixSRA(b269,Const(4))
    val x274 = save("x274", OpDef(op=FixAdd).addInput(lookup[OpDef]("x272"),x273).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [OpDef] x274 = FixAdd(x272,x273)
    val x278 = save("x278", BankedRead().bank(List(lookup[OpDef]("x325"))).offset(x274).setMem(lookup[SRAM]("x223")).en(Set(lookup[CounterValid]("b270"), lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).port(Some(1)).muxPort(0).broadcast(List(0)).castgroup(List(0)).tp(Fix(true, 32, 0)).sctx("OuterProduct.scala:58:42")) // [BankedRead] x278 = SRAMBankedRead(x223,Vector(List(x325)),Vector(x274),Vector(Set(b270, b252, b197, b170)),Vec[Fix[TRUE,_32,_0]])
    val x279 = save("x279", x278) // [BankedRead] x279 = VecApply(x278,0)
    val x280__1 = save("x280__1", x279) // [BankedRead] x280__1 = SimpleStruct(ArrayBuffer((_1,x279), (_2,Const(true))))
    val x280__2 = save("x280__2", Const(true).tp(Bool).sctx("OuterProduct.scala:58:42").name("_2")) // [Const] x280__2 = SimpleStruct(ArrayBuffer((_1,x279), (_2,Const(true))))
    val x281__1 = save("x281__1", MemWrite().setMem(lookup[FIFO]("x254__1")).en(Set(lookup[CounterValid]("b270"), lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).data(x280__1).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:58:42").name("_1")) // [MemWrite] x281__1 = StreamOutBankedWrite(x254,ArrayBuffer(x280),ArrayBuffer(Set(b270, b252, b197, b170)))
    val x281__2 = save("x281__2", MemWrite().setMem(lookup[FIFO]("x254__2")).en(Set(lookup[CounterValid]("b270"), lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170"))).data(x280__2).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).sctx("OuterProduct.scala:58:42").name("_2")) // [MemWrite] x281__2 = StreamOutBankedWrite(x254,ArrayBuffer(x280),ArrayBuffer(Set(b270, b252, b197, b170)))
    endState[Ctrl]
    val x283 = save("x283", FringeDenseStore(lookup[DRAM]("x164")).offset(MemRead().setMem(lookup[FIFO]("x253_offset"))).size(MemRead().setMem(lookup[FIFO]("x253_size"))).data(MemRead().setMem(lookup[FIFO]("x254__1"))).valid(MemRead().setMem(lookup[FIFO]("x254__2"))).ack(MemWrite().setMem(lookup[FIFO]("x255")).data).sctx("OuterProduct.scala:58:42")) // [FringeDenseStore] x283 = FringeDenseStore(x164,x253,x254,x255)
    val x286 = save("x286", createCtrl(schedule=Sequenced)(UnitController().en(Set(lookup[CounterValid]("b252"), lookup[CounterValid]("b197"), lookup[CounterValid]("b170")))).sctx("OuterProduct.scala:58:42")) // [UnitController] x286 = UnitPipe(Set(b252, b197, b170),Block(Const(())))
    val x284 = save("x284", MemRead().setMem(lookup[FIFO]("x255")).en(Set()).port(Some(0)).muxPort(0).broadcast(List(0)).castgroup(List(0)).tp(Bool).sctx("OuterProduct.scala:58:42")) // [MemRead] x284 = StreamInBankedRead(x255,ArrayBuffer(Set()))
    }; split12
    def split13 = {
    endState[Ctrl]
    endState[Ctrl]
    endState[Ctrl]
    endState[Ctrl]
    endState[Ctrl]
    }; split13
  }
}
