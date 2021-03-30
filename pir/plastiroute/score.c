#include "score.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"
#include "stdio.h"
#include "graph.h"
#include "move.h"
#include "routing.h"
#include "math.h"

extern routing_func routing_f;
/*
 * The grid is indexed moving right and down from the top left node. The
 * directions are: 0 
 *               3 N 1 
 *                 2 
 * From any node.  All DOR routes happen row-first, then column-wise.  
 */

int
linearize_link(int row, int col, int dir, int ncol) {
  return dir + 4 * (col + row * ncol);
}

int
linearize_node(int row, int col, int ncol) {
  return (col + row * ncol);
}

int
linearize_link_vc(int row, int col, int dir, int ncol, int vc) {
  return (dir + 4 * (col + row * ncol))*VC_OFFSET+vc;
}

void
print_throughput_limits(struct route_assignment_t *cand) {

}

int
route_comp_func(const struct route_t *a, const struct route_t *b) {
  /*printf("Compare (%d:%d->%d) against (%d:%d->%d)\n",
      a->route_id, a->prog_id_from, a->prog_id_to,
      b->route_id, b->prog_id_from, b->prog_id_to);*/
  
  if (a->route_id < b->route_id)
    return -1;
  else if (a->route_id > b->route_id)
    return 1;
  if (a->prog_id_to < b->prog_id_to)
    return -1;
  else if (a->prog_id_to > b->prog_id_to)
    return 1;
  if (a->prog_id_from < b->prog_id_from)
    return -1;
  else if (a->prog_id_from > b->prog_id_from)
    return 1;
  return 0;
}

void
add_sub_route_link(struct link_t *link, struct route_t *route, char add) {
  GList *it;
  struct route_t *tmp;
  char modify;
  assert(route);
  modify = 1;
  if (add) {
    /* If we're adding this route for the first time, then add the penalty.
     * Otherwise, don't add the penalty. This allows accounting for branch
     * routes with limited penalties, while keeping the complexity of the code
     * manageable. */
    for (it = link->routes; it; it=it->next) {
      tmp = it->data;
      if (tmp->route_id == route->route_id) {
        /*printf("Route already added!\n");*/
        modify = 0;
      }
    }
    if (modify) {
      link->total_weight += route->penalty;
      link->unique_dynamic++;
    }
    link->routes = g_list_prepend(link->routes, route);
  } else {
    assert(link->routes);
    assert(g_list_find_custom(link->routes, route, 
          (GCompareFunc)&route_comp_func) && "Attempt to remove nonexistent route");
    link->routes = g_list_remove(link->routes, route);
    /* Same as above, but only decrement the penalty if we've removed the last
     * link. */
    for (it = link->routes; it; it=it->next) {
      tmp = it->data;
      if (tmp->route_id == route->route_id)
        modify = 0;
    }
    if (modify) {
      link->total_weight -= route->penalty;
      link->unique_dynamic--;
    }
  }
  /*printf("Link route count: %d\n", g_list_length(link->routes));*/
}

GList          *
add_waits_hold(GList * graph, int from_buf, int to_buf,
	       struct route_t *route) {
  struct edge_t  *tmp;
  tmp = malloc(sizeof(struct edge_t));
  assert(tmp);

  tmp->from = from_buf;
  tmp->to = to_buf;
  tmp->ID = route;

  return g_list_prepend(graph, tmp);
}

int
get_buffer_direction(struct mesh_loc_t *from, struct mesh_loc_t *to) {
  assert((from->row == to->row) || (from->col == to->col));
  assert((from->row != to->row) || (from->col != to->col));

  if (to->row == from->row + 1) {
    return 2;
  } else if (to->row == from->row - 1) {
    return 0;
  } else if (to->col == from->col + 1) {
    return 1;
  } else if (to->col == from->col - 1) {
    return 3;
  }

  assert(0);
}

GList          *
add_arbitrary_waits_holds(GList * graph, struct node_t * src,
			  struct node_t * dst, struct route_t * route,
			  int ncol) {
  GList          *it;
  struct hop_t   *tmp;

  int             last_buf,
                  next_buf;

  last_buf = NODE_LINK_OFFSET + route->prog_id_from;

  for (it = route->route_hops; it; it = it->next) {
    tmp = it->data;
    /*
     * For each hop, in sequence, calculate the buffer that is waited on
     * for the hop. 
     */
    next_buf = linearize_link_vc(tmp->to.row, tmp->to.col,
			      get_buffer_direction(&tmp->from, &tmp->to),
			      ncol, route->vc);

    /*
     * Add a waits-holds link from the last buffer to the buffer that will 
     * be waited on in this network hop. This happens once per network
     * hop. 
     */
    graph = add_waits_hold(graph, last_buf, next_buf, route);
    last_buf = next_buf;
  }
  /*
   * After the last hop, we have to add a link waiting on the output
   * buffer. This is in addition to the hops in the network. 
   */
  if (dst->type != 0) {
    /*
     * Don't add this if it's the scalar output node, because that will
     * trivially form a cycle. 
     */
    next_buf = NODE_LINK_OFFSET + route->prog_id_to;
    graph = add_waits_hold(graph, last_buf, next_buf, route);
  }

  return graph;
}

void
set_route_vc(struct route_t *route, int vc) {
  struct hop_t *hop;
  GList *l;
  route->vc = vc;
  for (l=route->route_hops; l; l=l->next) {
    hop = l->data;
    hop->vc = vc;
  }
}

int 
allocate_vcs_seq(struct route_assignment_t *assignment) {
  int i;
  GList *it;
  struct route_t *tmp;

  i = 0;
  for (it = assignment->assigned_routes; it; it=it->next) {
    tmp = it->data;
    if (tmp->is_static)
      continue;
    set_route_vc(tmp, i++);
  }
  return i-1;
}

int
allocate_vcs_avoid_conflicts(struct route_assignment_t *assignment,
			     GList * conflicts) {
  GList          *route_it;
  GList          *edge_it;
  GList          *disallowed_vcs;
  GHashTable     *vc_assignments;
  struct route_t *route;
  struct edge_t  *edge;
  int             max_vc_assigned;
  int             next_neg_vc;
  int             vc;
  int            *conflicting_vc;
  int            vc_1,
                 vc_2;
  disallowed_vcs = route_it = edge_it = NULL;
  max_vc_assigned = -1;
  next_neg_vc = -1;
  /*
   * should only do this after all routes have been assigned
   */
  if (assignment->unassigned_routes)
    return 0;
  vc_assignments = g_hash_table_new(g_int_hash, g_int_equal);
  /*
   * First, iterate over routes and create entry corresponding to each
   * route key:route_ID, value:vc
   */
  for (route_it = assignment->assigned_routes; route_it;
       route_it = route_it->next) {
    route = route_it->data;
    route->vc = -1;
    g_hash_table_insert(vc_assignments, &route->route_id,
			GINT_TO_POINTER(route->vc));
  }
  /*
   * Iterate over routes again, this time to assign a vc to each 
   */
  for (route_it = assignment->assigned_routes; route_it;
       route_it = route_it->next) {
    route = route_it->data;

    if (route->is_static) {
      route->vc = next_neg_vc--;
      continue;
    }
    /*
     * Iterate over conflict list, and keep track of conflicting
     * (disallowed) vcs for this route 
     */
    for (edge_it = conflicts; edge_it; edge_it = edge_it->next) {
      edge = edge_it->data;
      if (edge->to == route->route_id) {
	conflicting_vc = g_hash_table_lookup(vc_assignments, &edge->from);
	disallowed_vcs = g_list_append(disallowed_vcs, conflicting_vc);
      } else if (edge->from == route->route_id) {
	conflicting_vc = g_hash_table_lookup(vc_assignments, &edge->to);
	disallowed_vcs = g_list_append(disallowed_vcs, conflicting_vc);
      }
    }
    vc = 0;
    /*
     * assign the lowest possible vc to this route
     */
    while (g_list_find(disallowed_vcs, GINT_TO_POINTER(vc)))
      vc++;
    assert(vc < NUM_VCS);
    set_route_vc(route, vc);
    g_hash_table_insert(vc_assignments, &route->route_id,
			GINT_TO_POINTER(route->vc));
    if (vc > max_vc_assigned)
      max_vc_assigned = vc;
    if (disallowed_vcs)
      g_list_free(disallowed_vcs);
    disallowed_vcs = NULL;
  }
  /*After we've allocated all VCs, check to make sure that all conflict constraints are satisfied*/
  for (edge_it = conflicts; edge_it; edge_it = edge_it->next) {
    edge = edge_it->data;
    vc_1 = GPOINTER_TO_INT(g_hash_table_lookup(vc_assignments,&edge->from));
    vc_2 = GPOINTER_TO_INT(g_hash_table_lookup(vc_assignments,&edge->to));
    /* Static routes can't conflict with other traffic classes. */
    if (vc_1 < 0)
      continue;
    if (vc_2 < 0)
      continue;
    assert(vc_1 != vc_2);
    assert(vc_1 >= 0);
    assert(vc_2 >= 0);
  }
  g_hash_table_destroy(vc_assignments);
  return max_vc_assigned;
}

void 
update_links(
  struct link_t *links,
  struct route_t *route,
  char add,
  int ncol
)
{
  GList *it;
  struct hop_t *hop;
  int dir;

  assert(route);
  /* Assert that this is not a control link. */
  assert(route->type != 0);
  assert(links);

  for(it = route->route_hops; it != NULL; it = it->next) {
    hop = (struct hop_t *) it->data;
    dir = get_buffer_direction(&hop->from, &hop->to);
    assert(links[linearize_link(hop->from.row, hop->from.col, dir, ncol)].total_weight >= 0);
    /*printf("%s route (%d/%p) to (%d,%d,%d)=%d\n", add ? "Adding" :  "Subtracting",
        route->route_id, (void*)route, hop->from.row, hop->from.col, dir,
        linearize_link(hop->from.row, hop->from.col, dir, ncol));*/
    add_sub_route_link(&links[linearize_link(hop->from.row, hop->from.col, dir, ncol)],
      route, add);
    assert(links[linearize_link(hop->from.row, hop->from.col, dir, ncol)].total_weight >= 0);
  }
}

int
allocate_vcs(struct route_assignment_t *assignment) {
  GList          *route_it;
  GList          *hop_it;
  struct route_t *route;
  struct hop_t   *hop;
  struct link_t  *link;
  int             vc,
                  i,
                  min_weight,
                  hops,
                  dir,
                  from_c,
                  to_c,
                  from_r,
                  to_r;
  hops = 0;
  vc = 0;
  if (assignment->unassigned_routes)
    return 0;

  for (route_it = assignment->assigned_routes; route_it;
       route_it = route_it->next) {
    route = route_it->data;
    for (hop_it = route->route_hops; hop_it; hop_it = hop_it->next) {
      hop = hop_it->data;
      from_c = hop->from.col;
      from_r = hop->from.row;
      to_c = hop->to.col;
      to_r = hop->to.row;

      if (from_r > to_r)
	dir = 0;
      else if (from_r < to_r)
	dir = 2;
      else if (from_c < to_c)
	dir = 3;
      else if (from_c > to_c)
	dir = 1;
      else
	dir = 5;
      assert(dir < 5);
      link =
	  &assignment->
	  links[linearize_link(from_r, from_c, dir, assignment->n_col)];
      min_weight = INT_MAX;
      /*
       * Allocate VC in link with lowest weight
       */
      for (i = 0; i < NUM_VCS; i++) {
	if (link->vc_weights[i] < min_weight) {
	  min_weight = link->vc_weights[i];
	  vc = i;
	}
      }
      link->vc_weights[vc] += route->penalty;
      hop->vc = vc;
      hops++;
    }
  }
  return hops;
}

void
recompute_link_weights(struct route_assignment_t *assign) {
  GList *it;
  struct route_t *tmp_route, *last_tmp;
  int i, dyn_w, stat_w, dyn_w_s, stat_w_s;

  for (i=0; i<4*assign->n_row*assign->n_col; i++) {
    assign->links[i].routes = g_list_sort(assign->links[i].routes,
        (GCompareFunc)route_comp_func);
    dyn_w = stat_w = dyn_w_s = stat_w_s = 0;
    tmp_route = NULL;
    for (it=assign->links[i].routes; it; it=it->next) {
      last_tmp = tmp_route;
      tmp_route = it->data;
      if (tmp_route->type == 0)
        continue;
      if (last_tmp && last_tmp->route_id == tmp_route->route_id)
        continue;
      if (tmp_route->type == 1) {
        if (tmp_route->is_static)
          stat_w_s += tmp_route->penalty;
        else
          dyn_w_s += tmp_route->penalty;
      } else  {
        if (tmp_route->is_static)
          stat_w += tmp_route->penalty;
        else
          dyn_w += tmp_route->penalty;
      }

    }
    assign->links[i].total_weight       = dyn_w   + stat_w + dyn_w_s + stat_w_s;
    assign->links[i].total_weight_scal  = dyn_w_s + stat_w_s;
    assign->links[i].static_weight      = stat_w  + stat_w_s;
    assign->links[i].static_weight_scal = stat_w_s;
  }

}

struct score_t *
score_route_assignment(struct route_assignment_t *cand) {
  struct score_t *ret;
  GList          *it;
  struct route_t *route;
  int             i,
                  total_links;
  uint64_t        total_hops;
  GHashTableIter  hash_it;
  void *key, *val;

  if (cand->unassigned_nodes)
    return NULL;
  if (cand->unassigned_routes)
    return NULL;

  total_links = 4 * cand->n_row * cand->n_col;

  ret = malloc(sizeof(struct score_t));
  assert(ret);
  memset(ret, 0, sizeof(*ret));

  g_hash_table_iter_init(&hash_it, cand->assigned_nodes);
  while (g_hash_table_iter_next(&hash_it, &key, &val)) {
    int this_inj, this_ej;
    struct node_t  *tmp = val;
    this_inj = this_ej = 0;

    for (it = cand->assigned_routes; it; it = it->next) {
      route = it->data;
      if (route->prog_id_to == route->prog_id_from)
        continue;
      if (route->is_static)
        continue;
      if (route->prog_id_to == tmp->prog_id)
        this_ej += route->penalty;
      if (route->prog_id_from == tmp->prog_id)
        this_inj += route->penalty;
    }

    if (this_inj > ret->inject_port_lim)
      ret->inject_port_lim = this_inj;
    if (this_ej > ret->eject_port_lim)
      ret->eject_port_lim = this_ej;
  }
  /*if (ret->inject_port_lim == 0)
    ret->inject_port_lim = 1;
  if (ret->eject_port_lim == 0)
    ret->eject_port_lim = 1;*/

  total_hops = 0;
  for (it = cand->assigned_routes; it; it = it->next) {
    route = it->data;

    if (route->type != 0) {
      if (route->is_static)
        ret->unmerged_static_hops += route->penalty * route->nhops;
      else
        ret->unmerged_dynamic_hops += route->penalty * route->nhops;
    }

    if (route->penalty > ret->link_penalty_lim)
      ret->link_penalty_lim = route->penalty;
    total_hops += route->penalty;
    if (route->nhops > ret->longest_route_hops && route->type == 2)
      ret->longest_route_hops = route->nhops;
  }
  ret->total_hops = total_hops;

  recompute_link_weights(cand);
  for (i = 0; i < total_links; i++) {
    int dyn_weight = cand->links[i].total_weight - cand->links[i].static_weight;
    int dyn_weight_scal = cand->links[i].total_weight_scal - cand->links[i].static_weight_scal;
    assert(dyn_weight >= 0);
    if (dyn_weight > ret->worst_link_tokens)
      ret->worst_link_tokens = dyn_weight;
    ret->total_route_hops += dyn_weight;
    ret->total_route_hops_scal += dyn_weight_scal;
    ret->merged_static_hops += cand->links[i].static_weight;
    ret->merged_static_hops_scal += cand->links[i].static_weight_scal;
  }

  ret->vcs_needed = allocate_vcs_hops(cand)+1;

  for (it = cand->assigned_routes; it; it=it->next) {
    route = it->data;
    if (route->is_static)
      continue;
    ret->tot_q += route->penalty;
    if (route->quality == 0)
      ret->tot_q0 += route->penalty;
    else if (route->penalty > ret->max_non_q0)
      ret->max_non_q0 = route->penalty;
  }

  return ret;
}

void
score_route(struct route_assignment_t *cand, struct route_t *route,
	    routing_func routing) {
  struct node_t  *from,
                 *to;
  GList *hops;
  struct mesh_loc_t src, dest;
  //printf("Score %d!\n", route->route_id);

  from = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_from);
  to = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_to);
  assert(from);
  assert(to);

#ifdef DBG
  printf("Scoring route from %d:(%d,%d) to %d:(%d,%d)\n",
	 route->prog_id_from, from->row, from->col,
	 route->prog_id_to, to->row, to->col);
#endif
  assert(routing == routing_f);

  /*
   * routing function is passed as an argument 
   */
  src.row = from->row;
  src.col = from->col;
  dest.row = to->row;
  dest.col = to->col;
  assert(route->fanout);
  if (route->fanout == 1)
    hops = routing(cand->links, src, dest, cand->n_col, cand->n_row, route);
  else
    hops = route_dor(cand->links, src, dest, cand->n_col, cand->n_row, route);
  route->nhops = g_list_length(hops);
  route->route_hops = hops;
  /* Only score for non-control routes. Control routes get a free static
   * assignment. */
  if (route->type != 0)
    update_links(cand->links, route, 1, cand->n_col);
}

void
unscore_route(struct route_assignment_t *cand, struct route_t *route,
	      routing_func routing) {
  struct node_t  *from,
                 *to;
  //printf("Unscore %d!\n", route->route_id);

  from = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_from);
  to = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_to);
  assert(from);
  assert(to);

#ifdef DBG
  printf("Un-scoring route from %d:(%d,%d) to %d:(%d,%d)\n",
	 route->prog_id_from, from->row, from->col,
	 route->prog_id_to, to->row, to->col);
#endif

  if (route->type != 0)
    update_links(cand->links, route, 0, cand->n_col);
  route->is_static = 0;
  g_list_free_full(route->route_hops, free);
  g_list_free_full(route->interm_nodes, free);
  route->route_hops = NULL;
  route->interm_nodes = NULL;
}

struct route_t *
dequeue_random_route_from_deadlock(GList ** graph) {
  struct edge_t  *tmp_edge;
  struct route_t *worst_route;
  int             candidates,
                  selected;

  candidates = g_list_length(*graph);
  selected = rand() % candidates;

  tmp_edge = g_list_nth_data(*graph, selected);
  worst_route = tmp_edge->ID;
  assert(worst_route);

  do {
    *graph = g_list_remove(*graph, worst_route);
  } while (g_list_find(*graph, worst_route));

  return worst_route;
}

struct route_t *
dequeue_worst_route_from_deadlock(GList ** graph) {
  struct edge_t  *tmp_edge;
  struct route_t *tmp_route;
  struct route_t *worst_route = NULL;
  int             worst_route_penalty = INT_MAX;
  GList          *l;

  for (l = *graph; l; l = l->next) {
    tmp_edge = l->data;
    tmp_route = tmp_edge->ID;
    if (tmp_route->penalty < worst_route_penalty) {
      worst_route_penalty = tmp_route->penalty;
      worst_route = tmp_route;
    }
  }

  assert(worst_route);

  do {
    *graph = g_list_remove(*graph, worst_route);
  } while (g_list_find(*graph, worst_route));

  return worst_route;
}

GList          *
get_deadlock_graph(struct route_assignment_t * cand) {
  GList          *it;
  GList          *graph = NULL;

  assert(!cand->unassigned_routes);
  for (it = cand->assigned_routes; it; it = it->next) {
    struct node_t  *from,
                   *to;
    struct route_t *route;
    route = it->data;
    /*
     * Add dependencies through the network. 
     */
    from = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_from);
    to = g_hash_table_lookup(cand->assigned_nodes, &route->prog_id_to);
    assert(from);
    assert(to);
    graph = add_arbitrary_waits_holds(graph, from, to, route, cand->n_col);
  }

  return graph;
}

GList *
get_program_graph(struct program_graph_t *prog) {
  GList          *it;
  GList          *graph = NULL;

  for (it = prog->unassigned_routes; it; it = it->next) {
    struct route_t *route;
    route = it->data;
    /*
     * Add a single dependence for ID->ID relationships. 
     */
    graph = add_waits_hold(graph, route->prog_id_from+NODE_LINK_OFFSET,
        route->prog_id_to+NODE_LINK_OFFSET, route);
  }

  return graph;

}

int
check_route_deadlock(struct route_assignment_t *cand) {
  int             deadlocks;
  GList          *graph = get_deadlock_graph(cand);

  deadlocks = ! !graph;
  g_list_free_full(graph, free);

  return deadlocks;
}

void
fix_route_deadlocks(struct route_assignment_t *cand) {
#if 0
  GList          *graph;
  int             tries_remaining = 5;
  struct route_t *worst_route = NULL;

  assert(0);

  graph = get_deadlock_graph(cand);

  graph = prune_to_cyclic(graph);
  if (!graph) {
    /*
     * Graph was deadlock free from the start. 
     */
    g_list_free_full(graph, free);
    return;
  }

  /*
   * Try a directed deadlock pruning algorithm, moving lightweight routes
   * to DOR. 
   */
  do {
    /*
     * Get the worst route (here, the lightweight route contributing to
     * the deadlock) and make it DOR. 
     */
    printf("Moving lightweight routes to DOR.\n");
    worst_route = dequeue_worst_route_from_deadlock(&graph);
    unscore_route(cand, worst_route, route_dor);
    assign_all_routes(cand, route_dor);
  } while ((graph = prune_to_cyclic(graph)));

  if (!check_route_deadlock(cand)) {
    printf("Fixed deadlock by directed DOR moves.\n");
    return;
  }

  while (tries_remaining) {
    graph = get_deadlock_graph(cand);
    graph = prune_to_cyclic(graph);
    do {
      /*
       * Get a random route and make it DOR. 
       */
      worst_route = dequeue_random_route_from_deadlock(&graph);
      unscore_route(cand, worst_route, route_dor);
      assign_all_routes(cand, route_dor);
    } while ((graph = prune_to_cyclic(graph)));

    g_list_free_full(graph, free);
    if (!check_route_deadlock(cand)) {
      printf("Fixed deadlock by random DOR moves.\n");
      return;
    }
  }

  /*
   * The nuclear option 
   */
  printf("Unable to fix deadlock.\n");
  unassign_all_routes(cand, route_dor);
  assign_all_routes(cand, route_dor);
#endif
}

#define SCORE_POW 1

float
score_func_linear(struct score_t *score, float *params) {
  float port_penalty;
  float vc_penalty;
  if (score->inject_port_lim > score->eject_port_lim)
    port_penalty = score->inject_port_lim;
  else
    port_penalty = score->eject_port_lim;

  if (score->vcs_needed <= params[5]) {
    vc_penalty = 0;
  } else {
    vc_penalty = pow(score->vcs_needed-params[5], SCORE_POW)*params[4];
  }

  return pow((score->worst_link_tokens/score->link_penalty_lim), SCORE_POW) * params[0]
       + pow((port_penalty/score->link_penalty_lim), SCORE_POW) * params[1]
       + pow(score->max_non_q0/score->link_penalty_lim, SCORE_POW) * params[1]
       + (score->longest_route_hops) * params[2]
       + (score->total_route_hops/score->total_hops) * params[3]
       + vc_penalty;
}
