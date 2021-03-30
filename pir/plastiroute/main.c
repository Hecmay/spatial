#include "stdio.h"
#include "assert.h"
#include "stdint.h"
#include "stdlib.h"
#include "time.h"
#include "getopt.h"
#include "string.h"

#include "types.h"
#include "score.h"
#include "move.h"
#include "gene.h"
#include "dot.h"
#include "float.h"
#include "graph.h"
#include "arbiter.h"
#include "routing.h"

routing_func    routing_f = NULL;

int
main(int argc, char **argv) {
  struct program_graph_t *prog;
  struct chip_graph_t *chip;
  struct route_assignment_t *route;
  struct cand_pool_t *cand;
  float           score;
  const char     *node_file,
                 *link_file,
                 *src_file,
                 *dst_file,
                 *summary_file,
                 *init_file,
                 *tungsten_file;
  int             iter,
                  pool,
                  topn,
                  count,
                  freeze,
                  i,
                  endpoint_vcs,
                  network_vcs,
                  xc,
                  yc,
                  use_dijkstra,
                  vc_test,
                  score_stop;
  int             row,
                  col,
                  stat,
                  scalar_static,
                  seconds,
                  pcu_pmu_ratio,
                  AGdup;
  float           target_vcs;
  char            ch;
  const char     *dot_file,
                 *place_file,
                 *run_key,
                 *pattern,
                 *tungsten_prefix;
  clock_t         start, end, diff;
#if 0
  GList          *deadlock_graph,
                 *vc_color_edges,
                 *logical_dep_graph;
#endif

  start = clock();

  srand(time(NULL));

  node_file = link_file = src_file = dst_file = init_file = dot_file = place_file = summary_file = tungsten_file = NULL;
  freeze = 1;
  iter = 10;
  pool = 10;
  topn = 2;
  count = 3;
  row = 16;
  col = 8;
  xc = yc = 1;
  target_vcs = 2;
  seconds = -1;
  vc_test = 0;
  scalar_static = stat = 0;
  routing_f = route_dor;
  run_key = "-";
  use_dijkstra = 25;
  pcu_pmu_ratio = 1;
  score_stop = -1;
  pattern = "checkerboard";
  tungsten_prefix = "";
  AGdup = 1;

  while ((ch = getopt(argc, argv, "y:z:A:X:E:P:VS:D:R:C:q:e:b:k:g:n:l:f:i:p:t:d:r:c:o:a:s:x:v:T:G:")) != -1) {
    switch (ch) {
      case 'y':
        src_file = optarg;
        break;
      case 'z':
        dst_file = optarg;
        break;
      case 'A':
        AGdup = atoi(optarg);
        break;
      case 'X':
        tungsten_prefix = optarg;
        break;
      case 'G':
        tungsten_file = optarg;
        break;
      case 'E':
        score_stop = atoi(optarg);
        break;
      case 'P':
        pcu_pmu_ratio = atoi(optarg);
        break;
    case 'V':
      vc_test = 1;
      break;
    case 'S':
      seconds = atoi(optarg);
      break;
    case 'D':
      use_dijkstra = atoi(optarg);
      break;
    case 'e':
      scalar_static = atoi(optarg);
      break;
    case 'q':
      target_vcs = atoi(optarg);
      break;
    case 'b':
      init_file = optarg;
      break;
    case 'v':
      summary_file = optarg;
      break;
    case 'k':
      run_key = optarg;
      break;
    case 'g':
      dot_file = optarg;
      break;
    case 'n':
      node_file = optarg;
      break;
    case 'l':
      link_file = optarg;
      break;
    case 'f':
      freeze = atoi(optarg);
      break;
    case 'p':
      pool = atoi(optarg);
      break;
    case 't':
      topn = atoi(optarg);
      break;
    case 'T':
      pattern = optarg;
      break;
    case 'd':
      count = atoi(optarg);
      break;
    case 'i':
      iter = atoi(optarg);
      break;
    case 'r':
      row = atoi(optarg);
      break;
    case 'c':
      col = atoi(optarg);
      break;
    case 'R':
      yc = atoi(optarg);
      break;
    case 'C':
      xc = atoi(optarg);
      break;
    case 'o':
      place_file = optarg;
      break;
    case 's':
      srand(atoi(optarg));
      break;
    case 'x':
      stat = atoi(optarg);
      break;
    case 'a': /* 'a' for algorithm (as in routing algorithm) */
      if(strncmp(optarg, "route_dor_XY", 32) == 0) { routing_f = route_dor_XY; }
      else if(strncmp(optarg, "route_dor_YX", 32) == 0) { routing_f = route_dor_YX; }
      else if(strncmp(optarg, "route_bdor", 32) == 0) { routing_f = route_bdor; }
      else if(strncmp(optarg, "route_min_local", 32) == 0) { routing_f = route_min_local; }
      else if(strncmp(optarg, "route_directed_valient", 32) == 0) { routing_f = route_directed_valient; }
      else if(strncmp(optarg, "route_min_directed_valient", 32) == 0) { routing_f = route_min_directed_valient; }
      else if(strncmp(optarg, "route_valient", 32) == 0) { routing_f = route_valient; }
      else if(strncmp(optarg, "route_min_valient", 32) == 0) { routing_f = route_min_valient; }
      else assert(0 && "unknown routing function");
      break;
    }
  }

  if (!node_file) {
    /* printf("Specify a node file.\n"); */
    assert(0);
  }

  if (!link_file) {
    /* fprintf(stderr, "Specify a link file.\n"); */
    assert(0);
  }

  printf("Max static: %dvec %dscal\n", stat, scalar_static);

  prog = load_program(node_file, link_file, src_file, dst_file);
  chip = new_chip(row, col, stat, scalar_static, xc, yc, target_vcs, pattern, pcu_pmu_ratio, AGdup);
  print_chip_graph(chip);
  assert(prog);
  assert(chip);

  if (prog->unassigned_routes == NULL) {
    printf("No routes provided!\n");
    // Create dummy placement file
    FILE           *out;
    out = fopen(tungsten_file, "w");
    assert(out);
    exit(0);
    fclose(out);
  }

  /*
  print_chip_graph(chip);
  print_prog_graph(prog);
  */

  route = new_route_assignment(prog, chip);
  assert(route);

  if (init_file)
    load_place_from_file(route, init_file);

  cand = init_candidate_pool(route, pool);
  assert(cand);
  set_score_func_by_name(cand, "linear");
  set_param(cand, 0, 250);
  set_param(cand, 1, 250);
  set_param(cand, 2, 0.5);
  set_param(cand, 3, 1);
  set_param(cand, 4, 500);
  set_param(cand, 5, target_vcs);

  init_place_all(cand);

  score = score_all(cand);

  clock_t stime;
  stime = clock();
  for (i = 0; i < iter; i++) {
    step_all(cand, routing_f, use_dijkstra);
    score = score_all(cand);

    summarize_scores(cand);
    cand = winnow_candidate_pool(cand, pool, topn, count, freeze);
    if (seconds > 0 && (clock() - stime)/CLOCKS_PER_SEC > seconds)
      break;
    /* Do a few more runs after our goal is reached, just to see if we can do
     * better. */
    if (score < score_stop)
      break;
  }

  print_static_dynamic_routes(cand->candidates[0]);

  /*
  printf("Evaluation parameters:\n"
	 "\tPool:\t\t%d\n"
	 "\tIterations:\t%d\n"
	 "\tTopN:\t\t%d\n"
	 "\tDup:\t\t%d\n"
	 "\tFreeze:\t\t%d\n", pool, iter, topn, count, freeze);

  printf("Evaluated %d candidate placements.",
	 pool * iter);
   */
  printf("Best score is %.2f\n", score);

#if 0
  vc_color_edges = add_endpoint_colors(cand->candidates[0]);
  endpoint_vcs = allocate_vcs_avoid_conflicts(cand->candidates[0], vc_color_edges);
#else
  if (vc_test) 
    endpoint_vcs = allocate_vcs_seq(cand->candidates[0]);
  else
    endpoint_vcs = allocate_vcs_hops(cand->candidates[0]);
#endif
  /* This sets a lower bound, even if we don't even run network vc allocation. */
  network_vcs = endpoint_vcs;
  printf("Used %d VCs for endpoint coloring.\n", endpoint_vcs + 1);
#if 0
  logical_dep_graph = get_program_graph(prog);
  logical_dep_graph = prune_to_cyclic(logical_dep_graph);
  vc_color_edges = NULL;
  for (i=1; ; i++) {
    printf("Deadlock breaking round: %d\n", i);
    deadlock_graph = get_deadlock_graph(cand->candidates[0]);
    deadlock_graph = prune_to_cyclic(deadlock_graph);
    if (deadlock_graph == NULL) {
      /*assert(!logical_dep_graph && "Network can't be cycle-free if logical graph isn't.");*/
      break;
 /*   } else if (deadlock_is_static(deadlock_graph)) {
      printf("Deadlock graph is cyclic, but is fully static.\n");
      printf("Assuming that input program is deadlock-free.\n");
      g_list_free_full(deadlock_graph, free);
      break;*/
    } else if (is_dependency_subgraph(deadlock_graph, logical_dep_graph)
        && !network_only_cycles(deadlock_graph)) {
      printf("Deadlock graph is cyclic, but is logical subgraph of program.\n");
      printf("Assuming that input program is deadlock-free.\n");
      g_list_free_full(deadlock_graph, free);
      break;
    }
    //assert(0);
    vc_color_edges = add_deadlock_colors(deadlock_graph, vc_color_edges);
    printf("\t%d edges for VC assignment.\n", g_list_length(vc_color_edges));
    network_vcs = allocate_vcs_avoid_conflicts(cand->candidates[0], vc_color_edges);

    printf("\tUsed %d VCs.\n", network_vcs + 1);
    deadlock_graph = get_deadlock_graph(cand->candidates[0]);
    deadlock_graph = prune_to_cyclic(deadlock_graph);
    printf("\t%d edges in cycles after deadlock VCs.\n", g_list_length(deadlock_graph)); 
    g_list_free_full(deadlock_graph, free);
  }
  g_list_free_full(logical_dep_graph, free);
  g_list_free_full(vc_color_edges, free);
#endif
  printf("\tUsed %d VCs.\n", network_vcs + 1);

  /*visit_all_pairs(cand->candidates[0]);
  print_arbiters(cand->candidates[0]);*/

  merge_route_hops(cand->candidates[0]);

  if (dot_file) {
    dot_print_assigned_routes(dot_file, cand->candidates[0]);
  }

  if (place_file) {
    place_print_assigned_nodes(place_file, cand->candidates[0]);
  }

  end = clock();
  diff = end - start;

  if (summary_file) {
    write_summary_line(summary_file, run_key, iter, cand,
        endpoint_vcs+1, network_vcs+1, diff*1000*1000/CLOCKS_PER_SEC);
  }
  if (tungsten_file) {
    write_tungsten_file(tungsten_file, cand->candidates[0], tungsten_prefix);
  }

  free_route_assignment(route);
  free_program(prog);
  free_chip(chip);
  free_candidate_pool(cand);

  /*
   * printf("Total links traversed (w/ penalty): %d\n",
   * score->total_route_hops); printf("Worst link (w/ penalty): %d\n",
   * score->worst_link_tokens); printf("Longest route (hops): %d\n",
   * score->longest_route_hops); 
   */

  return 0;

}
