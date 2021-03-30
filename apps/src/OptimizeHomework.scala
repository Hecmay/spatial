import Utils.toSpatialIntArray
import spatial.dsl._

@spatial object OptimizeHomework extends SpatialApp {
  def main(args: Array[String]): Unit = {
    //    val nItems = 32
    //    val maxSizeVal = 32
    //    val capacity = 127

    val nItems = 7
    val capacity = 15
    val solverValues = scala.Array(7, 9, 5, 12, 14, 6, 12)
    val solverSizes = scala.Array(3, 4, 2, 6, 7, 3, 5)

    val solver = new Solver(capacity, solverSizes, solverValues)
    solver.solveKnapsackIterative()
    solver.printDPArray()

    val sizes = toSpatialIntArray(solverSizes)
    val values = toSpatialIntArray(solverValues)

    val dpMatrix = DRAM[Int](nItems.to[I32], capacity.to[I32])
    val dSizes = DRAM[Int](nItems.to[I32])
    val dValues = DRAM[Int](nItems.to[I32])

    def getBucketDRAM = DRAM[Int](nItems.to[I32])

    val resultBuckets = getBucketDRAM
    val resultBucketSizes = getBucketDRAM
    val resultBucketValues = getBucketDRAM
    val resultNBuckets = ArgOut[Int]

    setMem(
      dpMatrix,
      Matrix.tabulate[Int](nItems.to[I32], (capacity + 1).to[I32])((_, _) =>
        -1.to[Int]))
    setMem(dSizes, sizes)
    setMem(dValues, values)

    Accel {
      // Step 1: generate the traversal matrix
      val nRows = nItems.to[I32]
      val nCols = (capacity + 1).to[I32]
      val step = 1.to[I32]
      val base = 0.to[I32]
      val sizes = SRAM[Int](nRows)
      val values = SRAM[Int](nRows)
      val DPArray = SRAM[Int](nRows, nCols)
      val ip = 4.to[I32]

      sizes load dSizes(base :: nRows par ip)
      values load dValues(base :: nRows par ip)
      DPArray load dpMatrix(base :: nRows, base :: nCols par ip)

      // Step 1: generate the DP matrix.
      Sequential.Foreach(nRows by step, nCols by step) { (nIdx, sIdx) =>
        val vn = values(nIdx)
        val sn = sizes(nIdx)
        val scanRowIdx = nIdx - 1.to[I32]
        val boundTest = scanRowIdx < base
        val pickTest = sn > sIdx
        val notPickedFromSizeLimitVal =
          mux(boundTest, base, DPArray(scanRowIdx, sIdx))
        val pickedCandidate = sIdx - sn
        val notPickedCandidate = sIdx
        val pickedVal = vn + mux(boundTest,
          base,
          DPArray(scanRowIdx, pickedCandidate))
        val notPickedVal =
          mux(boundTest, base, DPArray(scanRowIdx, notPickedCandidate))
        val canPickVal = max(pickedVal, notPickedVal)
        DPArray(nIdx, sIdx) =
          mux(pickTest, notPickedFromSizeLimitVal, canPickVal)
      }

      // Step 2: traverse the DP matrix backward.
      val traverseState = 0.to[I32]
      val doneState = 1.to[I32]
      val pickedBuckets = FIFO[Int](nRows)
      val pickedValues = FIFO[Int](nRows)
      val pickedSizes = FIFO[Int](nRows)

      val nIdx = Reg[Int](nRows - step)
      val sIdx = Reg[Int](nCols - step)
      val nextIdx = Reg[Int](nRows - step - step)
      val nBuckets = Reg[Int](base)
      FSM(traverseState)(state => state != doneState) { _ =>
        val currentScore = DPArray(nIdx, sIdx)
        val testPeekOOB = (nIdx - step) < base
        val peekScore = mux(testPeekOOB, base, DPArray(nextIdx, sIdx))
        if (currentScore != peekScore) {
          val sn = sizes(nIdx)
          val nIdxVal = nIdx.value
          pickedBuckets.enq(nIdxVal)
          pickedValues.enq(values(nIdxVal))
          pickedSizes.enq(sizes(nIdxVal))
          sIdx := sIdx.value - sn
          nBuckets := nBuckets.value + step
        }

        nIdx := nIdx.value - step
        nextIdx := nextIdx.value - step
      } { _ =>
        mux(nIdx.value >= base, traverseState, doneState)
      }

      resultNBuckets := nBuckets.value
      resultBuckets(base :: nRows) store pickedBuckets
      resultBucketSizes(base :: nRows) store pickedValues
      resultBucketValues(base :: nRows) store pickedSizes

    }

    val buckets = getMem(resultBuckets)
    val bucketSizes = getMem(resultBucketSizes)
    val bucketValues = getMem(resultBucketValues)
    val nBuckets = getArg(resultNBuckets)
    printArray(buckets, "buckets = ")
    printArray(bucketSizes, "bucketSizes = ")
    printArray(bucketValues, " bucketValues = ")
    println("nBuckets = ", nBuckets)

    //    val (bucketIDs, buckVals, buckSizes) = solver.reportPath()
    //    val bucketsGold = toSpatialIntArray(bucketIDs.toArray)
    //    val bucketValsGold = toSpatialIntArray(buckVals.toArray)
    //    val bucketSizesGold = toSpatialIntArray(buckSizes.toArray)

    //    def checkEql(a: Array[Int], b: Array[Int], bucketSize: Int) = {
    //      val diff = Array.tabulate[I32](bucketSize)(i => a(i) - b(i)).reduce { _ + _ }
    //      println("diff = ", diff)
    //      diff == 0.to[I32]
    //    }
    //    val pass = checkEql(bucketsGold, buckets, nBuckets) &&
    //      checkEql(bucketSizes, bucketSizesGold, nBuckets) &&
    //      checkEql(bucketValsGold, bucketValues, nBuckets)
    //    assert(pass, "OptimizeHomework (PASS)")
  }
}
