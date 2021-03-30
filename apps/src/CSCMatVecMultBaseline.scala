import spatial.dsl._

@spatial abstract class CSCMatVecMultBaseline(nVecRows: scala.Int,
                                              nnz: scala.Int,
                                              innerPar: scala.Int,
                                              tileSize: scala.Int,
                                              dataCFile: java.lang.String,
                                              indicesCFile: java.lang.String,
                                              indptrsCFile: java.lang.String,
                                              vecFile: java.lang.String,
                                              goldCFile: java.lang.String,
                                              bodyPar: scala.Int = 1)
  extends SpatialApp {
  def main(args: Array[String]): Unit = {
    val stPar, ldPar = innerPar

    val vector = DRAM[Int](nVecRows)
    val result = DRAM[Int](nVecRows)
    val indptrsH = DRAM[Int](nVecRows + 1)
    val indicesH = DRAM[Int](nnz)
    val dataH = DRAM[Int](nnz)

    val sVector = loadCSV1D[Int](vecFile)
    val sIndptrs = loadCSV1D[Int](indptrsCFile)
    val sIndices = loadCSV1D[Int](indicesCFile)
    val sData = loadCSV1D[Int](dataCFile)

    setMem(vector, sVector)
    setMem(indptrsH, sIndptrs)
    setMem(indicesH, sIndices)
    setMem(dataH, sData)

    printArray(sData, "data =")
    printArray(sVector, "vector = ")
    printArray(sIndices, "indices = ")
    printArray(sIndptrs, "indptrs = ")

    val partSz = math.ceil(nVecRows * 1.0 / bodyPar).toInt
    println("partSz = " + partSz)

    Accel {
      val vecTile = SRAM[Int](nVecRows)
      vecTile load vector(0 :: nVecRows par ldPar)
      val outTile = SRAM[Int](nVecRows)
      val indptrs = SRAM[Int](nVecRows + 1)
      // TODO: I should probably duplicate this one
      //  for each sub-matrix.
      indptrs load indicesH(0 :: nVecRows + 1 par ldPar)

      val resultBuffers = List.tabulate(bodyPar) { loopIdx =>
        val resultBuffer = SRAM[Int](nVecRows)
        Foreach (0 until nVecRows) { i =>
          resultBuffer(i) = 0
        }

        val start = loopIdx * partSz
        val end = math.min((loopIdx + 1) * partSz, nVecRows)
        scala.Console.println(s"start = $start, end = $end")

        // Work on the sub matrix
        Sequential.Foreach (start until end) { i =>
          val rowTile = SRAM[Int](tileSize)
          val dataTile = SRAM[Int](tileSize)

          val rowStart = indptrs(i)
          val rowEnd = indptrs(i + 1)

          rowTile load indicesH(rowStart :: rowEnd par ldPar)
          dataTile load dataH(rowStart :: rowEnd par ldPar)

          val valLen = rowEnd - rowStart
          val vecVal = vecTile(i)
          Sequential.Foreach (valLen by 1) { ii =>
            val addr = rowTile(ii)
            val oldResult = resultBuffer(addr)
            val newResult = oldResult + vecVal * dataTile(ii)
            resultBuffer(addr) = newResult
          }
        }

        resultBuffer
      }

      Sequential.Foreach (nVecRows by 1) { i =>
        outTile(i) = resultBuffers.map(m => m(i)).reduce{_+_}
      }

      result(0 :: nVecRows par stPar) store outTile
    }

    val arr = getMem(result)
    printArray(arr, "result = ")
    val gold = loadCSV1D[Int](goldCFile)
    printArray(gold, "gold = ")
    val cksum = approxEql(arr, gold, margin = 0)
    assert(cksum)
  }

}

object CSC_MatVecMultBaseline_1
  extends CSCMatVecMultBaseline(
    nVecRows = 32,
    nnz = 102,
    innerPar = 16,
    tileSize = 16,
    dataCFile = "/home/tianzhao/data/csc_matvec_small/dataC.csv",
    indicesCFile = "/home/tianzhao/data/csc_matvec_small/indicesC.csv",
    indptrsCFile = "/home/tianzhao/data/csc_matvec_small/indptrsC.csv",
    vecFile = "/home/tianzhao/data/csc_matvec_small/vector.csv",
    goldCFile = "/home/tianzhao/data/csc_matvec_small/goldC.csv",
    bodyPar = 1
  )
