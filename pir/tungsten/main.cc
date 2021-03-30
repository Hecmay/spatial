#include <iostream>
#include <fstream>
#include <istream>

#include "repl.h"

//#include "tests/dram_test.h"
//#include "tests/streamAB.h"
//#include "tests/pcu_add_test.h"
//#include "plasticine/tests/counter_test.h"
//#include "plasticine/tests/controller_test.h"
//#include "plasticine/tests/broadcast_test.h"
//#include "plasticine/tests/bankedsram_test.h"
//#include "plasticine/tests/nbuffer_test.h"
//#include "plasticine/tests/merge_test.h"
//#include "plasticine/tests/SumSquare/Top.h"
//#include "sparsity/templates/SparsePMU.h"
//#include "sparsity/test/Read.h"
//#include "sparsity/test/SparseRMW.h"
//#include "sparsity/test/Merge.h"
//#include "sparsity/test/MergeFollow.h"
//#include "sparsity/test/MergeTree.h"
//#include "sparsity/test/UnMergeTree.h"
//#include "sparsity/test/FollowerTree.h"
//
//#include "sparsity/test/DRAMRandom.h"
//#include "sparsity/test/Lock.h"
#include "plasticine/tests/CompressTest.h"

//#include "sparsity/apps/CSR_GEMV.h"
//#include "sparsity/apps/COO_GEMV.h"
//#include "sparsity/apps/PR.h"
//#include "sparsity/apps/BFS.h"

#include "sparsity/test/Random.h"
#include "sparsity/test/RandomRMW.h"
//#include "sparsity/test/OuterParRMW.h"
#include "sparsity/test/TestSparseDRAMExec.h"
#include "sparsity/test/TestSparseDRAMPar.h"
#include "sparsity/test/Scan.h"
#include "sparsity/test/MultiScan.h"
//#include "sparsity/test/LockRMW.h"
//#include "sparsity/test/TestSMU.h"

#include "tests/binlog/basic.h"

//CSR_GEMV<16> DUT("DUT");
//COO_GEMV<16> DUT("DUT");
//COO_GEMV<8> DUT("DUT");
//COO_GEMV<1> DUT("DUT");
//PR<8> DUT("DUT");
//BFS<8> DUT("DUT");
//TestOuterParRMW<0, TEST_OP, TEST_PMUS, 1> DUT("DUT");
//TestSMU<4096,16> DUT("DUT");

//LockTestRMW DUT("DUT");
// RandomTestRMW<4096,16,float> DUT("DUT");
//RandomTestRMW<4096,16,int> DUT("DUT");
// TestDRAMExec<float> DUT("DUT");
// TestDRAMPar<false,2,int,2> DUT("DUT");
// TestDRAMPar<true,1,int,1> DUT("DUT");
// TestDRAMExec<float> DUT("DUT");
//TestDRAMPar<8,float,8> DUT("DUT");
//TestDRAMPar<12,float,12> DUT("DUT");
//TestScan DUT("DUT");
// TestScan<16> DUT("DUT");
//TestMultiScan<2, 16> DUT("DUT");

TestBinlog DUT("DUT");

int main(int argc, char **argv) {

  REPL Top(&DUT, std::cout);
  if (argc == 2) {
    std::ifstream in(argv[1]);
    Top.Run(in, true);
  } else {
    Top.Run(std::cin);
  }
  std::ofstream logfile("hello.blog", std::ofstream::out|std::ofstream::binary);
  binlog::consume(logfile);
  return 0;
}
