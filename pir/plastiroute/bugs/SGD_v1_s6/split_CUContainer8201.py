from igraph import *
g = Graph(directed=True)
g.add_vertex("RetimingFIFO6193")
g.add_vertex("RetimingFIFO6238")
g.add_vertex("RetimingFIFO6271")
g.add_vertex("RetimingFIFO6304")
g.add_vertex("RetimingFIFO6337")
g.add_vertex("RetimingFIFO6370")
g.add_vertex("RetimingFIFO6403")
g.add_vertex("RetimingFIFO6436")
g.add_vertex("SelectByValid8092")
g.add_vertex("SelectByValid8095")
g.add_vertex("SelectByValid8099")
g.add_vertex("SelectByValid8103")
g.add_vertex("SelectByValid8107")
g.add_vertex("SelectByValid8111")
g.add_vertex("SelectByValid8115")
g.add_edge("RetimingFIFO6304", "SelectByValid8092")
g.add_edge("RetimingFIFO6436", "SelectByValid8092")
g.add_edge("SelectByValid8092", "SelectByValid8095")
g.add_edge("RetimingFIFO6370", "SelectByValid8095")
g.add_edge("SelectByValid8095", "SelectByValid8099")
g.add_edge("RetimingFIFO6271", "SelectByValid8099")
g.add_edge("SelectByValid8099", "SelectByValid8103")
g.add_edge("RetimingFIFO6337", "SelectByValid8103")
g.add_edge("SelectByValid8103", "SelectByValid8107")
g.add_edge("RetimingFIFO6403", "SelectByValid8107")
g.add_edge("SelectByValid8107", "SelectByValid8111")
g.add_edge("RetimingFIFO6193", "SelectByValid8111")
g.add_edge("SelectByValid8111", "SelectByValid8115")
g.add_edge("RetimingFIFO6238", "SelectByValid8115")
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
weights=[0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5]
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
with open("/home/yaqiz/pir/out/SGD_minibatch__D_64_N_16384_E_2_ts_1024_mp1_8_mp2_16_dynamic_v1_s6/CUContainer8201.cluster", 'w') as f:
    for v, mb in izip(g.vs, cluster.membership):
        f.write("{}={}\n".format(v["name"], mb))
