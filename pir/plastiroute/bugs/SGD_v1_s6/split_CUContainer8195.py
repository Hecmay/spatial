from igraph import *
g = Graph(directed=True)
g.add_vertex("RetimingFIFO5232")
g.add_vertex("RetimingFIFO5204")
g.add_vertex("RetimingFIFO5176")
g.add_vertex("RetimingFIFO5148")
g.add_vertex("RetimingFIFO5120")
g.add_vertex("OpDef2546")
g.add_vertex("OpDef2561")
g.add_vertex("OpDef2621")
g.add_vertex("OpDef2651")
g.add_vertex("OpDef2666")
g.add_vertex("OpDef2686")
g.add_edge("RetimingFIFO5120", "OpDef2546")
g.add_edge("RetimingFIFO5148", "OpDef2546")
g.add_edge("RetimingFIFO5176", "OpDef2561")
g.add_edge("RetimingFIFO5204", "OpDef2561")
g.add_edge("OpDef2546", "OpDef2621")
g.add_edge("OpDef2561", "OpDef2621")
g.add_edge("OpDef2621", "OpDef2651")
g.add_edge("OpDef2651", "OpDef2666")
g.add_edge("OpDef2666", "OpDef2686")
g.add_edge("RetimingFIFO5232", "OpDef2686")
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
weights=[0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5]
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
with open("/home/yaqiz/pir/out/SGD_minibatch__D_64_N_16384_E_2_ts_1024_mp1_8_mp2_16_dynamic_v1_s6/CUContainer8195.cluster", 'w') as f:
    for v, mb in izip(g.vs, cluster.membership):
        f.write("{}={}\n".format(v["name"], mb))
