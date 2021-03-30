
#include "state.h"
#include "counter.h"
#include "network.h"
#include "idealnetwork.h"
#include "staticnetwork.h"
#include "dram.h"

/*
 * Explicitly instantiate commonly used templates here so they can be pre-compiled. 
 * These templates are declared as extern in their template definition header so they are not
 * elaborated when imported. 
 * */

template class ValReadyPipeline<Token>;
template class FIFO<Token,16>;
template class FIFO<Token,2>;
template class FIFO<Token,1>;
template class Counter<1>;
template class Counter<16>;
template struct CheckedSend<Token>;
template struct CheckedReceive<Token>;
template struct CheckedSend<DRAMCommand>;
template struct CheckedReceive<DRAMCommand>;
template class DynamicNetwork<4, 8, 1>;
template class StaticNetwork<4, 1>;
// template class IdealNetwork<2>;
template class Broadcast<Token>;
