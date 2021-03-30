## File format

The program graph is in the format of 4 saperate csv files.

## node.csv : [node, type]

node: int

type: int
- 0: Host
- 1: Memory Controller Interface
- 2: PMU
- 3: DRAM Address Generator
- 4: PCU

The physical nodes in the network array are associated with types. A program node can only be mapped to a physical node if they have matching type. 

## link.csv : [output id, input id1, input id2, input id3, ...]
The edges can be broadcast links that have multiple desitinations. 
Links/flow are named by their output id

## inlink.csv: [input id, dst node id]
The input id and the destination node of that input

## outlink.csv: [output id, ctx id, src node id, link type, count]
ctx id: used for VC allocation. During VC allocation, multiple flow sharing the same physical link on the dynamic network need to be allocated with different VCs to avoid deadlock. But multiple links with the same ctx id can share the same VC. 

link type: 1 for scalar link and 2 for vector link

count: priority of link. Route the links with higher count first. 
