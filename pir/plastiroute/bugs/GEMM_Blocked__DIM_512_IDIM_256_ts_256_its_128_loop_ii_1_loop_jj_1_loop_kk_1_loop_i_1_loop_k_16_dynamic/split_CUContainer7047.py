from igraph import *
g = Graph(directed=True)
g.add_vertex("RetimingFIFO4685")
g.add_vertex("RetimingFIFO4720")
g.add_vertex("RetimingFIFO4755")
g.add_vertex("RetimingFIFO4790")
g.add_vertex("OpDef2207")
g.add_vertex("OpDef2222")
g.add_vertex("OpDef2312")
g.add_vertex("OpDef2357")
g.add_edge("RetimingFIFO4685", "OpDef2207")
g.add_edge("RetimingFIFO4720", "OpDef2207")
g.add_edge("RetimingFIFO4755", "OpDef2222")
g.add_edge("RetimingFIFO4790", "OpDef2222")
g.add_edge("OpDef2207", "OpDef2312")
g.add_edge("OpDef2222", "OpDef2312")
g.add_edge("OpDef2312", "OpDef2357")
def fix_dendrogram(graph, cl):
    already_merged = set()
    for merge in cl.merges:
        already_merged.update(merge)
    num_dendrogram_nodes = graph.vcount() + len(cl.merges)
    not_merged_yet = sorted(set(xrange(num_dendrogram_nodes)) - already_merged)
    if len(not_merged_yet) < 2:
        return
    v1, v2 = not_merged_yet[:2]
    cl._merges.append((v1, v2))
    del not_merged_yet[:2]
    missing_nodes = xrange(num_dendrogram_nodes,
            num_dendrogram_nodes + len(not_merged_yet))
    cl._merges.extend(izip(not_merged_yet, missing_nodes))
    cl._nmerges = graph.vcount()-1
weights=[0.5,0.5,0.5,0.5,0.5,0.5,0.5]
# plot(g)
dendrogram = g.community_walktrap(steps=10, weights=weights)
# dendrogram = g.community_fastgreedy(weights=weights)
# dendrogram = g.community_edge_betweenness(weights=weights)
# print(dendrogram.optimal_count)
fix_dendrogram(g, dendrogram)
cluster = dendrogram.as_clustering(n=2)
# cluster = g.community_infomap(edge_weights=weights)
# cluster = g.community_leading_eigenvector(clusters=2, weights=weights)
# cluster = g.community_label_propagation(weights=weights)
# cluster = g.community_multilevel(weights=weights)
# plot(cluster)
with open("/home/yaqiz/pir/out/GEMM_Blocked__DIM_512_IDIM_256_ts_256_its_128_loop_ii_1_loop_jj_1_loop_kk_1_loop_i_1_loop_k_16_dynamic/CUContainer7047.cluster", 'w') as f:
    for v, mb in izip(g.vs, cluster.membership):
        f.write("{}={}\n".format(v["name"], mb))
