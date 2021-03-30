#include "graph.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define MERGE_SAME_SENDER

struct gnode_t {
  int             ID;
  int             entering;
  int             leaving;
};

int max(int a, int b) {
  return a > b ? a : b;
}

int
hop_to_int(struct hop_t *hop, int nrow, int ncol) {
  int dir;
  switch(route_hop_to_direction(hop)) {
    default:
    case 'X': assert(0);
    case 'N': dir = 0; break;
    case 'E': dir = 1; break;
    case 'S': dir = 2; break;
    case 'W': dir = 3; break;
  }
  return dir+4*(hop->from.col+hop->from.row*max(nrow,ncol));
}

int
max_penalty_first(struct route_t *a, struct route_t *b) {
  if (a->penalty > b->penalty)
    return -1;
  if (a->penalty < b->penalty)
    return 1;
  return 0;
}

int
cmp_hop_equiv(struct hop_t *a, struct hop_t *b) {
  if (a->equiv_id < b->equiv_id)
    return -1;
  if (a->equiv_id > b->equiv_id)
    return 1;
  if (a->penalty > b->penalty)
    return -1;
  if (a->penalty < b->penalty)
    return 1;
  return 0;
}

int
prog_id_to_loc(int id, struct route_assignment_t *route) {
  struct node_t *tmp;
  tmp = g_hash_table_lookup(route->assigned_nodes, &id);
  assert(tmp);
  return tmp->col+tmp->row*max(route->n_row, route->n_col);
}

int
allocate_vcs_hops(struct route_assignment_t *route) {
  GList *r_it, *h_it;
  int max_vc;
  GList **hops_per_buf;
  int sq_dim;
  int max_prog_id, max_ctx, max_route_id;
  struct route_t *tmp_route;
  struct hop_t *tmp_hop;
  int *next_ej_vc;
  int *inj_vc_assigned;
  int i;
  assert(!route->unassigned_routes);
  sq_dim = max(route->n_row, route->n_col);
  hops_per_buf = calloc(sizeof(GList *), 4*sq_dim*sq_dim);
  assert(hops_per_buf);
  max_vc = -1;
  max_prog_id = max_ctx = -1;
  max_route_id = -1;
/*#ifndef MERGE_SAME_SENDER
  merge_route_hops(route);
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;

    if (tmp_route->prog_id_to > max_prog_id)
      max_prog_id = tmp_route->prog_id_to;
    if (tmp_route->prog_id_from > max_prog_id)
      max_prog_id = tmp_route->prog_id_from;
  }
  for (h_it=route->merged_hops; h_it; h_it=h_it->next) {
    tmp_hop = h_it->data;
    tmp_hop->equiv_id = 0;
    hops_per_buf[hop_to_int(tmp_hop, route->n_row, route->n_col)] =
      g_list_prepend(hops_per_buf[hop_to_int(tmp_hop, route->n_row, route->n_col)],
          tmp_hop);
  }
#else*/

  if (route->assigned_routes == NULL) 
    return -1;
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;

    if (tmp_route->prog_id_to >= max_prog_id)
      max_prog_id = tmp_route->prog_id_to+1;
    if (tmp_route->prog_id_from >= max_prog_id)
      max_prog_id = tmp_route->prog_id_from+1;
    if (tmp_route->ctx_id >= max_ctx)
      max_ctx = tmp_route->ctx_id+1;

    if (tmp_route->route_id >= max_route_id)
      max_route_id = tmp_route->route_id+1;

    if (tmp_route->is_static)
      continue;
    for (h_it = tmp_route->route_hops; h_it; h_it = h_it->next) {
      tmp_hop = h_it->data;
#ifdef MERGE_SAME_SENDER
      tmp_hop->equiv_id = tmp_route->prog_id_from;
#else
      tmp_hop->equiv_id = tmp_route->route_id;
#endif
      hops_per_buf[hop_to_int(tmp_hop, route->n_row, route->n_col)] =
        g_list_prepend(hops_per_buf[hop_to_int(tmp_hop, route->n_row, route->n_col)],
            tmp_hop);
    }
  }
  route->assigned_routes = g_list_sort(route->assigned_routes, (GCompareFunc)max_penalty_first);
/*#endif*/
  for (i=0; i<4*sq_dim*sq_dim; i++) {
    int last_vc;
//#ifdef MERGE_SAME_SENDER
    int last_equiv_id = -1;
    hops_per_buf[i] = g_list_sort(hops_per_buf[i], (GCompareFunc)cmp_hop_equiv);
//#endif
    last_vc = -1;
    for (h_it = hops_per_buf[i]; h_it; h_it = h_it->next) {
      tmp_hop = h_it->data;
//#ifdef MERGE_SAME_SENDER
      if (tmp_hop->equiv_id != last_equiv_id) {
        last_equiv_id = tmp_hop->equiv_id;
//#endif
        last_vc++;
//#ifdef MERGE_SAME_SENDER
      }
//#endif
      tmp_hop->vc = last_vc;
      if (last_vc > max_vc)
        max_vc = last_vc;
    }
    g_list_free(hops_per_buf[i]);
  }
  free(hops_per_buf);
  next_ej_vc = calloc(sizeof(int), sq_dim*sq_dim);
  assert(next_ej_vc);
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;
    if (tmp_route->is_static)
      continue;
    tmp_route->ej_vc = next_ej_vc[prog_id_to_loc(tmp_route->prog_id_to,
                                                      route)]++;
  }
  free(next_ej_vc);

  next_ej_vc = 0;
  assert(max_prog_id > 0);
  next_ej_vc = calloc(sizeof(int), sq_dim*sq_dim);
  assert(next_ej_vc);
  inj_vc_assigned = malloc(sizeof(int)*max_route_id);
  assert(inj_vc_assigned);
  for (i=0; i<max_route_id; i++) {
    inj_vc_assigned[i] = -1;
  }
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;
    if (tmp_route->is_static)
      continue;
    //int l = prog_id_to_loc(tmp_route->prog_id_from, route);
    if (inj_vc_assigned[tmp_route->route_id] == -1) {
      inj_vc_assigned[tmp_route->route_id] = 
              next_ej_vc[prog_id_to_loc(tmp_route->prog_id_from, route)]++;
    }
    tmp_route->vc = inj_vc_assigned[tmp_route->route_id];
  }
  free(inj_vc_assigned);
  free(next_ej_vc);
  
  int *max_vc_assigned = calloc(sizeof(int),(max_route_id+1));
  assert(max_vc_assigned);
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;
    if (tmp_route->is_static)
      continue;

  /*  if (tmp_route->vc > max_vc_assigned[tmp_route->route_id])
      max_vc_assigned[tmp_route->route_id] = tmp_route->vc;
    if (tmp_route->ej_vc > max_vc_assigned[tmp_route->route_id])
      max_vc_assigned[tmp_route->route_id] = tmp_route->ej_vc;*/
    for (h_it = tmp_route->route_hops; h_it; h_it=h_it->next) {
      tmp_hop = h_it->data;
      if (tmp_hop->vc > max_vc_assigned[tmp_route->route_id])
        max_vc_assigned[tmp_route->route_id] = tmp_hop->vc;
    }
  }
  for (r_it=route->assigned_routes; r_it; r_it=r_it->next) {
    tmp_route = r_it->data;
    if (tmp_route->is_static)
      continue;

    tmp_route->quality = max_vc_assigned[tmp_route->route_id];
  }
  free(max_vc_assigned);

  return max_vc;
}


GList          *
prune_to_cyclic(GList * graph) {
  int             removed;
  GHashTable     *deletable_from,
                 *deletable_to,
                 *nodes;
  GHashTableIter  hash_it;
  GList          *it;

  deletable_from =
      g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
  deletable_to =
      g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
  nodes = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, free);
  assert(nodes);

  /*
   * Build a table of all nodes. 
   */
  for (it = graph; it; it = it->next) {
    struct edge_t  *dat;
    struct gnode_t *tmp;
    dat = it->data;
    if (!g_hash_table_lookup(nodes, &dat->from)) {
      tmp = malloc(sizeof(struct gnode_t));
      memset(tmp, 0, sizeof *tmp);
      tmp->ID = dat->from;
      assert(g_hash_table_insert(nodes, &tmp->ID, tmp));
    }
    if (!g_hash_table_lookup(nodes, &dat->to)) {
      tmp = malloc(sizeof(struct gnode_t));
      memset(tmp, 0, sizeof *tmp);
      tmp->ID = dat->to;
      assert(g_hash_table_insert(nodes, &tmp->ID, tmp));
    }
  }

  /*
   * printf("Checking graph of length %d\n", g_list_length(graph));
   */

  do {
    struct gnode_t *tmp_from,
                   *tmp_to,
                   *tmp_node;
    struct edge_t  *dat;
    removed = 0;

    g_hash_table_iter_init(&hash_it, nodes);
    while (g_hash_table_iter_next(&hash_it, NULL, (void **) &tmp_node))
      tmp_node->entering = tmp_node->leaving = 0;

    for (it = graph; it; it = it->next) {
      dat = it->data;
      assert(tmp_from = g_hash_table_lookup(nodes, &dat->from));
      assert(tmp_to = g_hash_table_lookup(nodes, &dat->to));
      tmp_from->leaving++;
      tmp_to->entering++;
    }

    g_hash_table_iter_init(&hash_it, nodes);
    while (g_hash_table_iter_next(&hash_it, NULL, (void **) &tmp_node)) {
      assert(tmp_node);
      if (tmp_node->entering && tmp_node->leaving)
	continue;
      if (!tmp_node->leaving)
	g_hash_table_insert(deletable_to, &tmp_node->ID, tmp_node);
      if (!tmp_node->entering)
	g_hash_table_insert(deletable_from, &tmp_node->ID, tmp_node);
    }
    it = graph;
    while (it) {
      GList          *next;
      next = it->next;
      dat = it->data;
      if (g_hash_table_lookup(deletable_from, &dat->from)
	  || g_hash_table_lookup(deletable_to, &dat->to)) {
	free(it->data);
	graph = g_list_delete_link(graph, it);
	removed++;
      }
      it = next;
    }

    /*
     * printf("Pruning graph. Removed %d edges (%d remaining).\n",
     * removed, g_list_length(graph));
     */

  } while (removed);

  g_hash_table_unref(deletable_from);
  g_hash_table_unref(deletable_to);
  g_hash_table_unref(nodes);

  return graph;
}

GList          *
get_shared_edges(GList * graph, struct edge_t * edge) {
  GList          *it,
                 *ret;
  struct edge_t  *tmp;

  assert(edge);

  ret = NULL;
  for (it = graph; it; it = it->next) {
    tmp = it->data;
    if ((  tmp->from == edge->to ||  tmp->to == edge->from)
	&& tmp->ID != edge->ID)
      ret = g_list_prepend(ret, tmp);
  }

  return ret;
}

int
count_shared_edges(GList * graph, struct edge_t *edge) {
  GList          *it;
  struct edge_t  *tmp;
  int             ret;

  ret = 0;
  for (it = graph; it; it = it->next) {
    tmp = it->data;
    if ((tmp->from == edge->from || tmp->to == edge->to)
	&& tmp->ID != edge->ID)
      ret++;
  }

  return ret;
}

struct edge_t *
get_rand_break_point(GList *graph) {
  int sel;

  sel = rand() % g_list_length(graph);

  return (struct edge_t*) g_list_nth_data(graph, sel);
}

struct edge_t  *
get_loc_break_point(GList * graph, int last_loc) {
  struct edge_t  *tmp;
  GList          *it;
  tmp = NULL;

  for (it = graph; it; it = it->next) {
    tmp = it->data;
    if (tmp->to == last_loc)
      return tmp;
  }

  return NULL;
}

struct edge_t  *
get_best_break_point(GList * graph) {
  struct edge_t  *ret,
                 *tmp;
  GList          *it;
  int             this_count;
  int             cur_min = -INT_MAX;
  ret = NULL;
  tmp = NULL;

  for (it = graph; it; it = it->next) {
    tmp = it->data;
    this_count = count_shared_edges(graph, tmp);
    /*
     * Get the minimum hop that's in the network (not a connecting edge
     * within a node. 
     */
    if (this_count > cur_min
	/*
	 * && tmp->from < NODE_LINK_OFFSET
	 */
	&& tmp->to < NODE_LINK_OFFSET) {
      cur_min = this_count;
      ret = tmp;
    }
  }

  assert(ret);
  return ret;
}

/* This function iterates over all pairs in the graph, looking for a pair of 
 * edges that have a waits-holds dependence, are not part of the same route,
 * and are allocated in the same VC. This is an "underspecified" dependence, 
 * because it would violate the VC allocation constraint that will be added. */
struct edge_t *
get_underspecified_break_point(GList *graph) {
  GList *it_a, *it_b;
  struct edge_t *tmp_a, *tmp_b;
  struct route_t *rt_a, *rt_b;

  for (it_a = graph; it_a; it_a=it_a->next) {
    tmp_a = it_a->data;
    rt_a = tmp_a->ID;
    /* Traversing the entire graph allows only comparing on one direction on
     * each traversal. */
    for (it_b = graph; it_b; it_b=it_b->next) {
      tmp_b = it_b->data;
      rt_b = tmp_b->ID;
      if (tmp_a->to == tmp_b->from && rt_a->route_id != rt_b->route_id 
          && rt_a->vc > 0 && rt_b->vc > 0 && rt_a->vc != rt_b->vc)
        return tmp_a;
    }
  }
  return NULL;
}

GList          *
add_edge_nodup(GList * graph, int from, int to, void *ID) {
  GList          *it;
  struct edge_t  *new,
                 *tmp;

  for (it = graph; it; it = it->next) {
    tmp = it->data;
    if (tmp->from == from && tmp->to == to)
      return graph;
  }

  new = malloc(sizeof(*new));
  assert(new);
  new->from = from;
  new->to = to;
  new->ID = ID;
  return g_list_prepend(graph, new);
}

void
print_graph(GList * graph, const char *prefix) {
  struct edge_t  *tmp;
  struct route_t *tmp_r;
  for (; graph; graph = graph->next) {
    tmp = graph->data;
    tmp_r = tmp->ID;

    printf("%s%d->%d\t[label=\"%p=%d,%d,%c\"]\n", prefix, tmp->from, tmp->to, tmp->ID,
	   tmp_r ? tmp_r->route_id : 0,
           tmp_r ? tmp_r->vc : 0,
           tmp_r ? tmp_r->is_static ? 's' : 'd' : 'x');
  }
}

GList *
add_endpoint_colors(struct route_assignment_t *route) {
  GList *it, *it_n;
  GList *ret;
  struct route_t *A, *B;
  struct node_t *nA, *nB;
  /*struct arbiter_t *tmp_arb;*/
  
  ret = NULL;

  /* printf("Visiting pairs of routes to same endpoint:\n"); */
  for (it = route->assigned_routes; it; it=it->next) {
    A = it->data;
    nA = g_hash_table_lookup(route->assigned_nodes, &A->prog_id_to);
    for (it_n = it->next; it_n; it_n = it_n->next) {
      B = it_n->data;
      nB = g_hash_table_lookup(route->assigned_nodes, &B->prog_id_to);
      assert(nA && nB);
      /*if (A->penalty == 1
          && B->penalty == 1)
        continue;*/
      if (nA->row == nB->row && nA->col == nB->col) {
        /* printf("\t%d and %d to %d\n", A->route_id, B->route_id, A->prog_id_to); */
        if (A->route_id < B->route_id)
          ret = add_edge_nodup(ret, A->route_id, B->route_id, NULL);
        else if (A->route_id > B->route_id)
          ret = add_edge_nodup(ret, B->route_id, A->route_id, NULL);

        /*tmp_arb = first_common_ancestor(A, B, route);
        route->arbiters = g_list_prepend(route->arbiters, tmp_arb);*/
      } else {
        assert(A->prog_id_to != B->prog_id_to);
      }
    }
  }
  return ret;
}      

int 
deadlock_is_static(GList *deadlock) {
  GList *it;
  struct edge_t *edge;
  struct route_t *rt;
  for (it=deadlock; it; it=it->next) {
    edge = it->data;
    rt = edge->ID;
    if (!rt->is_static)
      return 0;
  }
  return 1;
}

GList          *
add_deadlock_colors(GList * waits_holds, GList * colors) {
  int             added;
  /*int             last_loc;*/
  struct edge_t  *tmp;
  struct route_t *A,
                 *B;
  GList          *it,
                 *l;

  added = 0;
  /*last_loc = 0;*/
   printf("Adding constraints to graph with %d edges.\n",
	 g_list_length(waits_holds));
  while ((waits_holds = prune_to_cyclic(waits_holds))) {
    printf("Cyclic subgraph with %d edges.\n", g_list_length(waits_holds));
    
    print_graph(waits_holds, "\t");

    /*tmp = get_loc_break_point(waits_holds, last_loc);*/
    /*tmp = get_rand_break_point(waits_holds);*/
    /*tmp = get_underspecified_break_point(waits_holds);*/
    /*assert(tmp);*/
    /*if (!tmp)*/
      tmp = get_rand_break_point(waits_holds);
    /*printf("Best break point: %d->%d (%p)\n", tmp->from, tmp->to, tmp->ID);*/
    l = get_shared_edges(waits_holds, tmp);
    A = tmp->ID;
    for (it = l; it; it = it->next) {
      void *colors_old = colors;
      B = ((struct edge_t *) it->data)->ID;
      /*
       * assert(A->route_id != B->route_id);
       */
      if (A->route_id < B->route_id)
	colors = add_edge_nodup(colors, A->route_id, B->route_id, NULL);
      else if (A->route_id > B->route_id)
	colors = add_edge_nodup(colors, B->route_id, A->route_id, NULL);
      if (colors != colors_old)
        added++;
    }
    g_list_free(l);
    if (!added)
      continue;
    /*printf("Total of %d constraints added.\n", added);*/
    waits_holds = g_list_remove(waits_holds, tmp);
  }

  g_list_free_full(waits_holds, free);
  
  /*printf("Final constraint graph:\n");*/
  /*print_graph(colors, "\t");*/
  

  return colors;
}

int
network_only_cycles(GList *deadlock) {
  GList *copy, *it;
  struct edge_t *tmp_edge;
  copy = NULL;
  for (it=deadlock; it; it=it->next) {
    tmp_edge = it->data;
    if (tmp_edge->from > NODE_LINK_OFFSET || tmp_edge->to > NODE_LINK_OFFSET)
      continue;
    copy = add_edge_nodup(copy, tmp_edge->from, tmp_edge->to, tmp_edge->ID);
  }
  copy = prune_to_cyclic(copy);
  if (!copy)
    return 0;
  g_list_free_full(copy, free);
  return 1;
}

int
is_dependency_subgraph(GList *deadlock, GList *program) {
  GHashTable *prog_nodes;
  GHashTable *frontier;
  GHashTableIter prog_node_iter;
  GHashTableIter frontier_iter;
  GList *it;
  struct edge_t *edge;
  int *prog_node, *discard, *test_node;

  prog_nodes = g_hash_table_new(g_int_hash, g_int_equal);

  printf("Checking if physical cycles are program cycles.\n");
  /* Find all program nodes in the deadlock. */
  for (it=deadlock; it; it=it->next) {
    edge = it->data;
    if (edge->to >= NODE_LINK_OFFSET) {
      if (g_hash_table_add(prog_nodes, &edge->to))
        printf("\tAdding node: %ld\n", edge->to-NODE_LINK_OFFSET);
    } else if (edge->from >= NODE_LINK_OFFSET) {
      if (g_hash_table_add(prog_nodes, &edge->from))
        printf("\tAdding node: %ld\n", edge->from-NODE_LINK_OFFSET);
    }
  }

  /* Brute-force BFS on the deadlock for each program node, to find preceding
   * program nodes. */
  g_hash_table_iter_init(&prog_node_iter, prog_nodes);
  while (g_hash_table_iter_next(&prog_node_iter, (void*)&prog_node, (void*)&discard)) {
    int updated;
    printf("Doing reverse BFS for destination %d.\n", *prog_node);
    frontier = g_hash_table_new(g_int_hash, g_int_equal);
    for (it=deadlock; it; it=it->next) {
      edge = it->data;
      edge->tmp = 0;
    } 
    /*g_hash_table_add(frontier, prog_node);*/
    do {
      updated = 0;
      for (it=deadlock; it; it=it->next) {
        edge = it->data;
        /* Edge has been processed. */
        if (edge->tmp)
          continue;
        /* Don't continue past program nodes. */
        if (edge->to >= NODE_LINK_OFFSET &&
            edge->to != *prog_node)
          continue;
        /* If the destination reaches the currently-tried node, add the node. */
        if (!(g_hash_table_contains(frontier, &edge->to)
              || edge->to == *prog_node))
          continue;
        if (g_hash_table_add(frontier, &edge->from))
          updated++;
      } 
    } while (updated);

    /* Loop over the hash table. */
    g_hash_table_iter_init(&frontier_iter, frontier);
    while (g_hash_table_iter_next(&frontier_iter, (void*)&test_node, (void*)&discard)) {
      /* No dependency from node to itself. */
      /*if (*test_node == *prog_node)
        continue;*/
      /* Don't look at network nodes. */
      if (*test_node < NODE_LINK_OFFSET)
        continue;
      printf("\tFound dependence: %d on %d\n", *test_node, *prog_node);
      for (it=program; it; it=it->next) {
        edge = it->data;
        if (edge->from == *test_node && edge->to == *prog_node)
          goto test_success;
      }
      printf("Could not resolve dependence!\n");
      g_hash_table_destroy(frontier);
      g_hash_table_destroy(prog_nodes);
      return 0;
    }
test_success:
    g_hash_table_destroy(frontier);
  }



  /* No conflicting nodes found. */
  g_hash_table_destroy(prog_nodes);
  return 1;
  
}
