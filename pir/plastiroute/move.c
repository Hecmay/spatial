#include "move.h"
#include "types.h"
#include "score.h"
#include "graph.h"

#include "dot.h"
#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "routing.h"

#include "heap.h"

#define DYNAMIC_STEP 100
//#define DBG_DIJKSTRA

extern routing_func routing_f;

int
route_cmp(const struct route_t *a,
          const struct route_t *b) {
  if (a->route_id < b->route_id)
   return -1;
  if (a->route_id > b->route_id)
    return 1;
  return 0;
}

int
assign_node_random(struct route_assignment_t *assign) {
  struct node_t  *p_node,
                 *c_node;
  int             type;
  int             candidates;

  if (!assign->unassigned_nodes) {
    return 0;
  }

  p_node = g_list_first(assign->unassigned_nodes)->data;
  assert(p_node);
  type = p_node->type;
  assert(type < NODE_TYPES);
  assign->unassigned_nodes =
      g_list_remove(assign->unassigned_nodes, p_node);

  candidates = g_list_length(assign->free_nodes[type]);
  if (!candidates) {
    printf("No free nodes of type %d for node %d\n", type, p_node->prog_id);
    assert(0);
  }

  c_node = g_list_nth(assign->free_nodes[type], rand() % candidates)->data;
  assert(c_node);
  assign->free_nodes[type] =
      g_list_remove(assign->free_nodes[type], c_node);

  /*
   * printf("Assigning program node %d to (%d,%d) (%d candidates)\n",
   * p_node->prog_id, c_node->row, c_node->col, candidates); 
   */

  c_node->prog_id = p_node->prog_id;
  c_node->name = p_node->name;
  free(p_node);

  g_hash_table_insert(assign->assigned_nodes, &c_node->prog_id, c_node);

  return 1;
}

int
node_manhattan_distance(struct node_t *A, struct node_t *B) {
  int             x,
                  y;
  x = A->row - B->row;
  y = A->col - B->col;
  x = x > 0 ? x : -x;
  y = y > 0 ? y : -y;
  return x + y;
}

void
load_place_from_file(struct route_assignment_t *assign, const char *file) {
  FILE *in;
  char typ; 
  int id, place;
  struct node_t *p_node, *c_node, *tmp_node;
  GList *it;

  in = fopen(file, "r");
  assert(in);

  while (1 == fscanf(in, " %c", &typ)) {
    if (typ != 'N') {
      printf("Skipping line type: %c\n", typ);
      assert(0 == fscanf(in, "%*[^\n]\n"));
      continue;
    }
    assert(2 == fscanf(in, "%d %d", &id, &place));
    printf("Load id %d at %d\n", id, place);
    p_node = NULL;
    for (it=assign->unassigned_nodes; it; it=it->next) {
      tmp_node = it->data;
      if (tmp_node->prog_id == id) {
        p_node = tmp_node;
        break;
      }
    }
    if (!p_node) {
      printf("Could not find program node: %d!\n", id);
      continue;
    }
    c_node = NULL;
    for (it=assign->free_nodes[p_node->type]; it; it=it->next) {
      tmp_node = it->data;
      if (dot_lin_row_col(tmp_node->row, tmp_node->col, assign->n_row, assign->n_col)
          == place)
      //if (tmp_node->row*assign->n_col+tmp_node->col == place)
        c_node = tmp_node;
    }
    if (!c_node) {
      printf("Could not find node of type %d at %d!\n", p_node->type,
          place);
      continue;
    }

    c_node->prog_id = p_node->prog_id;
    assign->free_nodes[c_node->type] =
        g_list_remove(assign->free_nodes[c_node->type], c_node);


    assign->unassigned_nodes = 
      g_list_remove(assign->unassigned_nodes, p_node);
    free(p_node);
    g_hash_table_insert(assign->assigned_nodes, &c_node->prog_id, c_node);
  }
}

int
assign_node_directed(struct route_assignment_t *assign) {
  struct node_t  *p_node,
                 *c_node,
                 *cand_node;
  int             type;
  struct route_t *route;
  GList          *connected,
                 *l,
                 *ll;
  int             best_hops,
                  this_hops;

  if (!assign->unassigned_nodes) {
    return 0;
  }

  p_node = g_list_first(assign->unassigned_nodes)->data;
  assert(p_node);
  type = p_node->type;
  assert(type < NODE_TYPES);
  assign->unassigned_nodes =
      g_list_remove(assign->unassigned_nodes, p_node);

  /*
   * Build a list of all routes connecting to this node. 
   */
  connected = NULL;
  for (l = assign->assigned_routes; l; l = l->next) {
    route = l->data;
    if (route->type != 2)
      continue;
    if (route->penalty < 10)
      continue;
    if (route->prog_id_from == p_node->prog_id) {
      cand_node = g_hash_table_lookup(assign->assigned_nodes,
				      &route->prog_id_to);
      if (cand_node)
	connected = g_list_prepend(connected, cand_node);
    } else if (route->prog_id_to == p_node->prog_id) {
      cand_node = g_hash_table_lookup(assign->assigned_nodes,
				      &route->prog_id_from);
      if (cand_node)
	connected = g_list_prepend(connected, cand_node);
    }
  }
  for (l = assign->unassigned_routes; l; l = l->next) {
    route = l->data;
    if (route->prog_id_from == p_node->prog_id) {
      cand_node = g_hash_table_lookup(assign->assigned_nodes,
				      &route->prog_id_to);
      if (cand_node)
	connected = g_list_prepend(connected, cand_node);
    } else if (route->prog_id_to == p_node->prog_id) {
      cand_node = g_hash_table_lookup(assign->assigned_nodes,
				      &route->prog_id_from);
      if (cand_node)
	connected = g_list_prepend(connected, cand_node);
    }
  }

  c_node = NULL;
  best_hops = INT_MAX;
  for (l = assign->free_nodes[type]; l; l = l->next) {
    this_hops = 0;
    cand_node = l->data;
    for (ll = connected; ll; ll = ll->next) {
      this_hops += node_manhattan_distance(ll->data, cand_node);
    }
    if (this_hops <= best_hops) {
      c_node = cand_node;
      best_hops = this_hops;
    }
  }

  if (!c_node)
    fprintf(stderr, "Could not find a physical node for program node: %s\n", p_node->name);
  assert(c_node);
  assign->free_nodes[type] =
      g_list_remove(assign->free_nodes[type], c_node);

  c_node->prog_id = p_node->prog_id;
  free(p_node);

  g_hash_table_insert(assign->assigned_nodes, &c_node->prog_id, c_node);
  g_list_free(connected);

  return 1;

}

/* This function merges all route hops into a single, unique set of multicast
 * routing decisions. */
void
merge_route_hops(struct route_assignment_t *route) {
  GList *it, *rt, *hops;
  struct hop_t *tmp_hop, *ref_hop;
  struct route_t *tmp_route;
  g_list_free(route->merged_hops);
  g_list_free(route->merged_hops_static);
  route->merged_hops = NULL;
  route->merged_hops_static = NULL;
  assert(!route->unassigned_routes);
  /* Iterate over all the assigned program routes, and add them. */
  for (rt = route->assigned_routes; rt; rt = rt->next) {
    tmp_route = rt->data;
    /*printf("Merge route %d (static=%d)\n", tmp_route->route_id,
        tmp_route->is_static);*/
    for (hops = tmp_route->route_hops; hops; hops = hops->next) {
      tmp_hop = hops->data;
      /*printf("\tHop: (%d,%d)->(%d,%d)\n", tmp_hop->from.row, tmp_hop->from.col,
                                          tmp_hop->to.row, tmp_hop->to.col);*/
      tmp_hop->is_static = tmp_route->is_static;
      tmp_hop->route_id = tmp_route->route_id;
      tmp_hop->type = tmp_route->type;
      tmp_hop->penalty = tmp_route->penalty;
      /* Check if we already have this ID reaching this destination. */
      if (!tmp_hop->is_static) {
        tmp_hop->to.row   /= route->yc;
        tmp_hop->from.row /= route->yc;
        tmp_hop->to.col   /= route->xc;
        tmp_hop->from.col /= route->xc;
        if (tmp_hop->to.row == tmp_hop->from.row
            && tmp_hop->to.col == tmp_hop->from.col)
          goto skip_hop;
        for (it = route->merged_hops; it; it=it->next) {
          ref_hop = it->data;
          /* We can skip adding tmp_hop. Goto the next iteration of the 
           * surrounding for loop to try another tmp_hop. */
          if ((ref_hop->route_id == tmp_hop->route_id)
            && (ref_hop->to.row == tmp_hop->to.row)
            && (ref_hop->to.col == tmp_hop->to.col))
            goto skip_hop;
        }
      } else {
        for (it = route->merged_hops_static; it; it=it->next) {
          ref_hop = it->data;
          assert(ref_hop->is_static);
          /* We can skip adding tmp_hop. Goto the next iteration of the 
           * surrounding for loop to try another tmp_hop. */
          if ((ref_hop->route_id == tmp_hop->route_id)
            && (ref_hop->to.row == tmp_hop->to.row)
            && (ref_hop->to.col == tmp_hop->to.col))
            goto skip_hop;
        }
      }
      /* We cannot skip adding tmp_hop. */
      if (tmp_route->is_static)
        route->merged_hops_static = g_list_prepend(route->merged_hops_static, tmp_hop);
      else
        route->merged_hops = g_list_prepend(route->merged_hops, tmp_hop);
skip_hop:
      ;
    }
  }
}

void
clear_static_routes(struct route_assignment_t *assign) {
  int i;
  GList *it;
  struct route_t *tmp_route;

  /*assert(!assign->unassigned_routes);*/

  for (i=0; i<4*assign->n_row*assign->n_col; i++) {
    memset(assign->links[i].unique_static, 0, sizeof(assign->links[i].unique_static));
    assign->links[i].static_weight = 0;
    /*assign->links[i].routes = g_list_sort(assign->links[i].routes,
        (GCompareFunc)route_comp_func);*/
  }

  for (it=assign->unassigned_routes; it; it=it->next) {
    tmp_route = it->data;
    tmp_route->is_static = 0;
  }

  for (it=assign->assigned_routes; it; it=it->next) {
    tmp_route = it->data;
    tmp_route->is_static = 0;
  }
}

int
route_penalty_sort_helper(const struct route_t *a, const struct route_t *b) {
  int a_fo_sq, b_fo_sq;
  a_fo_sq = a->fanout*a->fanout;
  b_fo_sq = b->fanout*b->fanout;
  if (a->penalty*a_fo_sq > b->penalty*b_fo_sq)
    return -1;
  if (a->penalty*a_fo_sq < b->penalty*b_fo_sq)
    return 1;
  return 0;
}

void
move_route_to_static(struct route_assignment_t *assign, int route_id, int free, int type_id) {
  GList *it;
  struct route_t *tmp_route;
  int i;
  int any_touched;
  int is_control;
  is_control = 0;
  for (it=assign->assigned_routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->route_id == route_id)
      tmp_route->is_static = 1;
    /* Control routes are free. */
    if (tmp_route->type == 0)
      is_control = 1;
  }
  for (it=assign->unassigned_routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->route_id == route_id)
      tmp_route->is_static = 1;
    /* Control routes are free. */
    if (tmp_route->type == 0)
      is_control = 1;
  }
  if (is_control)
    return;
  for (i=0; i<4*assign->n_col*assign->n_row; i++) {
    any_touched = 0;
    for (it=assign->links[i].routes; it; it=it->next) {
      tmp_route = it->data;
      assert(tmp_route);
      if (tmp_route->route_id == route_id) {
        any_touched = 1;
        break;
      }
    }
    if (any_touched) {
      assign->links[i].static_weight += tmp_route->penalty;
      assign->links[i].unique_dynamic--;
      if (!free)
        assign->links[i].unique_static[type_id]++;
    }
  }
}

int
route_can_be_made_static(struct link_t *link, int route_id, int max_static, int type_id, int pre) {
  GList *it;
  struct route_t *tmp_route;
  /*printf("%d unique %d max %d static %d dyn\n", link->unique_static, max_static,
      g_list_length(link->static_routes),
      g_list_length(link->routes));*/
  /*assert(link->unique_static[type_id] <= max_static);*/
  /*if (link->static_routes)
    assert(link->unique_static);*/
  /* Can't make a route static twice. */
  for (it=link->routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->route_id == route_id && tmp_route->is_static) {
      printf("Attempting to make route static twice (%d)!\n", route_id);
      return 1;
    }
  }
  /* The link is not fully statically used. */
  if (link->unique_static[type_id] < max_static)
    return 1;
  if (pre)
    return 0;
  /* The links is fully statically used, and the route is present on it. */
  for (it=link->routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->route_id == route_id) {
#ifdef DBG_DIJKSTRA
      printf("\nStatic fail for %d on %d (%d,%d,%d) (t=%d, unique %d)", route_id, link->id, link->row, link->col, link->dir, type_id, link->unique_static[type_id]);
      for (it=link->routes; it; it=it->next) {
        tmp_route = it->data;
        printf("\t%d", tmp_route->route_id);
      }
      printf("\n");
#endif
      return 0;
    }
  }
  /* The route is not present in the link. This link poses no problem. */
  return 1;
}

void
try_assign_static_route(struct route_assignment_t *assign, struct route_t *route) {
  int is_free, i, succeed;
#ifdef DBG_DIJKSTRA
  printf("Trying to move route %d to static.\n", route->route_id);
#endif
  is_free = 0;
  if (route->type == 0)
    is_free = 1;
  if (route->ag_mc_link && route->nhops == 0)
    is_free = 1;
  if (route->argin_link)
    is_free = 1;
  if (route->is_static)
    return;
  succeed = 1;
  if (!is_free) {
    for (i=0; i<4*assign->n_row*assign->n_col; i++) {
      if (route->type == 2) {
        if (!route_can_be_made_static(&(assign->links[i]), route->route_id, 
              assign->max_static, 0, 0))
          succeed = 0;
      } else {
        if (!route_can_be_made_static(&(assign->links[i]), route->route_id, 
              assign->max_static_scalar, 1, 0))
          succeed = 0;
      }
    }
  }
  if (!succeed)
    return;
#ifdef DBG_DIJKSTRA
  printf("Route to static: %d\n", route->route_id);
#endif
  move_route_to_static(assign, route->route_id, is_free, route->type != 2);
}


void
greedy_assign_static_routes(struct route_assignment_t *assign) {
  GList *it;
  /*struct route_t *tmp_route;*/

  assert(!assign->unassigned_routes);
  //clear_static_routes(assign);
  assign->assigned_routes = g_list_sort(assign->assigned_routes,
      (GCompareFunc)route_penalty_sort_helper);
  /*for (it=assign->assigned_routes; it; it=it->next) {
    tmp_route = it->data;
    //assert(!tmp_route->is_static);
  }*/
  /*printf("Starting static route assignment.\n");*/
  for (it=assign->assigned_routes; it; it=it->next) {
    try_assign_static_route(assign, it->data);
  }
}

void
unassign_all_routes(struct route_assignment_t *assign,
		    routing_func routing) {
  GList          *it;

  for (it = assign->assigned_routes; it; it = it->next)
    unscore_route(assign, it->data, routing);

  /*
   * Once non-DOR routes are allowed, this will have to ensure that all
   * routes are changed to DOR prior to assignment. 
   */
  if (!assign->unassigned_routes) {
    assign->unassigned_routes = assign->assigned_routes;
    assign->assigned_routes = NULL;
    return;
  }

  assign->unassigned_routes = g_list_concat(assign->assigned_routes,
					    assign->unassigned_routes);

  assign->assigned_routes = NULL;
}

static unsigned int hash_seed;

unsigned int 
fnv_1a(unsigned int x) {
  unsigned i, ret;
  ret = 0x811c9dc5;
  for (i=0; i<4; i++) {
    ret ^= x&0xFF;
    ret *= 16777619;
    x >>= 8;
  }
  return  ret;
}

int
fanout_first(struct route_t *A, struct route_t *B) {
#if 0
  if (A->fanout > B->fanout) {
    return -1;
  } else if (A->fanout < B->fanout) {
    return 1;
  }
#endif
#if 1
  if (A->fanout*A->penalty > B->fanout*B->penalty)
    return -1;
  else if (A->fanout*A->penalty < B->fanout*B->penalty)
    return 1;
#else
  if (A->penalty > B->penalty)
    return -1;
  else if (A->penalty < B->penalty)
    return 1;
#endif

#if 1
  int Ah, Bh;
  Ah = fnv_1a(A->route_id);
  Bh = fnv_1a(B->route_id);
  /*printf("%d, %d -> %d, %d\n", A->route_id, B->route_id, Ah, Bh);*/
  if (Ah > Bh)
    return -1;
  else if (Ah < Bh)
    return 1;
#endif
  if (A->route_id > B->route_id)
    return -1;
  else if (A->route_id < B->route_id)
    return 1;

  return 0;
}

int
get_hop_cost(struct route_assignment_t *assign, int this_r, int this_c, int dir,
    int type_class, int static_ref, int nc) {

  if (assign->links[linearize_link(this_r, this_c, dir, nc)].unique_static[type_class] < static_ref)
    return 1;

#if 0
  return 1001;
#else
  return DYNAMIC_STEP*(1+(assign->links[linearize_link(this_r, this_c, dir, nc)].total_weight
                -assign->links[linearize_link(this_r, this_c, dir, nc)].static_weight)/assign->max_penalty) + 1;
#endif
}

int *
dijkstra_get_branch_costs(struct route_assignment_t *assign, int src_id, 
    GList *dst_ids, int *output_costs, int type, int route_id) {
  int *dists;
  int *visited;
  int *traceback;
  struct node_t  *node;
  int num_nodes, i, nr, nc, this_r, this_c;
  int type_class, static_ref;

  struct heap h;

  heap_create(&h, 0);

  if (type == 2) {
    type_class = 0;
    static_ref = assign->max_static;
  } else {
    type_class = 1;
    static_ref = assign->max_static_scalar;
  }


  nr = assign->n_row;
  nc = assign->n_col;
  num_nodes = nr*nc;
  dists = malloc(sizeof(int)*num_nodes);
  visited = malloc(sizeof(int)*num_nodes);
  traceback = malloc(sizeof(int)*num_nodes);
  assert(dists && visited);
  for (i=0; i<num_nodes; i++) {
    dists[i] = INT_MAX;
    visited[i] = 0;
    traceback[i] = -1;
  }

  /* The first node to visit is the source node. */
  assert((node = g_hash_table_lookup(assign->assigned_nodes, &src_id)));
  this_r = node->row;
  this_c = node->col;

#ifdef DBG_DIJKSTRA
  printf("Performing static allocation with maximum: %d\n", static_ref);
  printf("\t%dx%d=%d grid (row*col)\n", nr, nc, num_nodes);
  printf("\tOriginal node: %d=(%d,%d)\n",
      linearize_node(this_r, this_c, nc),
      this_r, this_c);
#endif

  dists[linearize_node(this_r, this_c, nc)] = 0;

  while (1) {
    int cur_dist, delta, hop;
    //printf("\tUpdating distance for node (%d,%d)\n", this_r, this_c);
    /* Mark the current node as visited. */
    visited[linearize_node(this_r, this_c, nc)] = 1;
    cur_dist = dists[linearize_node(this_r, this_c, nc)];
    assert(linearize_node(this_r, this_c, nc) < num_nodes);
    if (this_r != 0) {
      /* Check up */
      hop = linearize_node(this_r-1, this_c, nc);
      delta = get_hop_cost(assign, this_r, this_c, 0, type_class, static_ref, nc);
      assert(delta > 0);
      //delta = (assign->links[linearize_link(this_r, this_c, 0, nc)].unique_static[type_class] >= static_ref) ? 1000 : 1;
#ifdef DBG_DIJKSTRA 
      if (delta < 10)
        assert(route_can_be_made_static(&assign->links[linearize_link(this_r, this_c, 0, nc)], route_id, static_ref, type_class, 1));
#endif
      if (!visited[hop] && dists[hop] > cur_dist+delta) {
        //printf("Insert into heap (%d,%d)\n", cur_dist+delta, hop);
        heap_insert(&h, cur_dist+delta, hop);
        dists[hop] = cur_dist+delta;
        traceback[hop] = 2;
      }
    }
    if (this_r != nr-1) {
      /* Check down */
      hop = linearize_node(this_r+1, this_c, nc);
      delta = get_hop_cost(assign, this_r, this_c, 2, type_class, static_ref, nc);
      assert(delta > 0);
      //delta = (assign->links[linearize_link(this_r, this_c, 2, nc)].unique_static[type_class] >= static_ref) ? 1000 : 1;
#ifdef DBG_DIJKSTRA
      if (delta < 10)
        assert(route_can_be_made_static(&assign->links[linearize_link(this_r, this_c, 2, nc)], route_id, static_ref, type_class, 1));
#endif
      if (!visited[hop] && dists[hop] > cur_dist+delta) {
        //printf("Insert into heap (%d,%d)\n", cur_dist+delta, hop);
        heap_insert(&h, cur_dist+delta, hop);
        dists[hop] = cur_dist+delta;
        traceback[hop] = 0;
      }
    }
    if (this_c != 0) {
      /* Check left */
      hop = linearize_node(this_r, this_c-1, nc);
      delta = get_hop_cost(assign, this_r, this_c, 3, type_class, static_ref, nc);
      assert(delta > 0);
      //delta = (assign->links[linearize_link(this_r, this_c, 3, nc)].unique_static[type_class] >= static_ref) ? 1000 : 1;
#ifdef DBG_DIJKSTRA 
      if (delta < 10)
        assert(route_can_be_made_static(&assign->links[linearize_link(this_r, this_c, 3, nc)], route_id, static_ref, type_class, 1));
#endif
      if (!visited[hop] && dists[hop] > cur_dist+delta) {
        //printf("Insert into heap (%d,%d)\n", cur_dist+delta, hop);
        heap_insert(&h, cur_dist+delta, hop);
        dists[hop] = cur_dist+delta;
        traceback[hop] = 1;
      }
    }
    if (this_c != nc-1) {
      /* Check right */
      hop = linearize_node(this_r, this_c+1, nc);
      delta = get_hop_cost(assign, this_r, this_c, 1, type_class, static_ref, nc);
      assert(delta > 0);
      //delta = (assign->links[linearize_link(this_r, this_c, 1, nc)].unique_static[type_class] >= static_ref) ? 1000 : 1;
#ifdef DBG_DIJKSTRA 
      if (delta < 10)
        assert(route_can_be_made_static(&assign->links[linearize_link(this_r, this_c, 1, nc)], route_id, static_ref, type_class, 1));
#endif
      if (!visited[hop] && dists[hop] > cur_dist+delta) {
        //printf("Insert into heap (%d,%d)\n", cur_dist+delta, hop);
        heap_insert(&h, cur_dist+delta, hop);
        dists[hop] = cur_dist+delta;
        traceback[hop] = 3;
      }
    }
    int next_hop, dist;
    next_hop = -1;
    while (heap_delmin(&h, &dist, &next_hop)) {
      //printf("Read from heap (%d,%d)\n", dist, next_hop);
      if (!visited[next_hop]) {
        this_c = next_hop % nc;
        this_r = next_hop/nc;
        break;
      } else {
        //printf("Duplicate-inserted hop!\n");
        next_hop = -1;
      }
    }
    if (next_hop < 0)
      break;
 }

  for (i=0; i<num_nodes; i++) {
    assert(dists[i] < INT_MAX);
    assert(visited[i]);
    /* We can't trace back from the original node! */
    if (dists[i])
      assert(traceback[i] >= 0);
  }

#ifdef DBG_DIJKSTRA
  int j;
  printf("Traceback matrix:\n");
  for (i=0; i<nr; i++) {
    for (j=0; j<nc; j++) {
      printf("%c", /*dists[linearize_node(i, j, nc)],*/
                       (traceback[linearize_node(i, j, nc)] ==  0) ? '^' :
                       (traceback[linearize_node(i, j, nc)] ==  2) ? 'v' :
                       (traceback[linearize_node(i, j, nc)] ==  3) ? '<' :
                       (traceback[linearize_node(i, j, nc)] ==  1) ? '>' :
                       (traceback[linearize_node(i, j, nc)] == -1) ? '0' : '?');
    }
    printf("\n");
  }

#endif

  i=0;
  for (; dst_ids; dst_ids=dst_ids->next) {
    assert((node = g_hash_table_lookup(assign->assigned_nodes, dst_ids->data)));
    this_r = node->row;
    this_c = node->col;
    output_costs[i++] = dists[linearize_node(this_r, this_c, nc)];
#ifdef DBG_DIJKSTRA
    if (output_costs[i-1] > 1000) {
      printf("Could not find static path to destination node!\n");
    }
#endif
  }

  free(dists);
  free(visited);
  heap_destroy(&h);
  
  return traceback;
}

/* This is an approximate broadcast algorithm. What it does is start with the
 * root node, and then pick a new node with the best weight to add to the
 * current node set at each step. */
int *
approx_arbor(int num_nodes, int *edges, int src_id, GList *dst_ids, int **_hops, int **_order) {
  int *prev_node;
  int *prev_node_val;
  int to, from, min_to, min_from, min_val, in_set;
  int *hops,
      *order;
#ifdef DBG_DIJKSTRA
  int i;
#endif

  prev_node = calloc(sizeof(int), num_nodes);
  assert(prev_node);
  prev_node_val = calloc(sizeof(int), num_nodes);
  assert(prev_node_val);
  hops = calloc(sizeof(int), num_nodes);
  assert(hops);
  order = calloc(sizeof(int), num_nodes);
  assert(order);
  /* The root node is by definition in the reachable set. */
  in_set = 0;
  prev_node[0] = -1;
  prev_node_val[0] = 1;
  hops[0] = 0;
  order[in_set++] = 0;

  while (in_set != num_nodes) {
    /* Find the minimum value over the set of all possible next choices. */
    min_val = INT_MAX;
    min_to = min_from = -1;
    for (to = 0; to < num_nodes; to++) {
      /* If we have reached this node already, don't try to reach it again. */
      if (prev_node_val[to])
        continue;
      for (from = 0; from <num_nodes; from++) {
        /* If we haven't reached the potential source yet, we can't try it. */
        if (!prev_node_val[from])
          continue;
        /* The edge to check does not exist. */
        if (edges[to+from*num_nodes] < 0)
          continue;
        if (edges[to+from*num_nodes] < min_val) {
          min_val = edges[to+from*num_nodes];
          min_to = to;
          min_from = from;
        }
      }
    }
    /* We have to have found something. */
    assert(min_val != INT_MAX);
    /* assert(min_to != src_id); */
    assert(!prev_node[min_to]);
    prev_node[min_to] = min_from;
    if (min_from > 0) {
      assert(prev_node[min_from] >= 0);
      assert(prev_node_val[min_from]);
    }
    assert(min_from < num_nodes);
    hops[min_to] = hops[min_from]+min_val;
    prev_node_val[min_to] = 1;
    order[in_set++] = min_to;
    /*printf("\tAdd hop with lat=%d from (%d:%d) to (%d:%d)\n",
        min_val,
        min_from ? *(int*)g_list_nth_data(dst_ids, min_from-1) : src_id, min_from,
        min_to ? *(int*)g_list_nth_data(dst_ids, min_to-1) : src_id, min_to);*/
  }
#ifdef DBG_DIJKSTRA
  printf("Final graph:\ndigraph bcast{");
  for (i=0; i<num_nodes; i++) {
    if (prev_node_val[order[i]])
      printf("\t%d -> %d [label=\"%d\"];\n", prev_node[order[i]], order[i], hops[order[i]]);
    }
  printf("}\n");
#endif
  free(prev_node_val);
  *_hops = hops;
  *_order = order;
  return prev_node;
}

#if 0
void
edmonds_iteration(int num_nodes, int *edges, int src_id, GList *dst_ids) {
  int to, from;
  int *new_edges;
  int *pi_v;
  int i, j;
  GList *it;
  new_edges = calloc(sizeof(int), num_nodes*num_nodes);
  assert(new_edges);
  for (i=0; i<num_nodes*num_nodes; i++) {
    new_edges[i] = -1;
  }
  pi_v = calloc(sizeof(int), num_nodes);
  assert(pi_v);
  for (to=0; to<num_nodes; to++) {
    int min = INT_MAX;
    pi_v[to] = -1;
    for (from=0; from<num_nodes; from++) {
      if (edges[to+from*num_nodes] > 0
          && edges[to+from*num_nodes] < min) {
        min = edges[to+from*num_nodes];
        pi_v[to] = from;
      }
    }
    if (pi_v[to] >= 0) {
      for (from=0; from<num_nodes; from++) {
        if (edges[to+from*num_nodes] < 0)
          continue;
        new_edges[to+from*num_nodes] = edges[to+from*num_nodes] - min;
      }
    }
  }
  printf("\n%7s:%6d", "From/To", src_id);
  for (it=dst_ids; it; it=it->next)
    printf("%6d", *(int*)it->data);
  printf("\n%7d:", src_id);
  for (i=0; i<num_nodes; i++)
    printf("%6d", new_edges[i]);
  it = dst_ids;
  for (j=1; j<num_nodes; j++) {
    printf("\n%7d:", *(int*)it->data);
    for (i=0; i<num_nodes; i++)
      printf("%6d", new_edges[i+j*(num_nodes)]);
    it=it->next;
  }
  printf("\n");
}
#endif

void
print_static_dynamic_routes(struct route_assignment_t *assign) {
  GList *it;
  struct route_t *tmp_route;
  printf("All static routes:\n");
  for (it=assign->assigned_routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->is_static)
      printf("\t%d:%d:%d", tmp_route->route_id, tmp_route->fanout, tmp_route->penalty);
  }
  printf("\nAll dynamic routes:\n");
  for (it=assign->assigned_routes; it; it=it->next) {
    tmp_route = it->data;
    if (!tmp_route->is_static)
      printf("\t%d:%d:%d", tmp_route->route_id, tmp_route->fanout, tmp_route->penalty);
  }
  printf("\n");
}

/* Returns 0 if not static; else returns 1 if can be made static. */
int
do_fanout_route(struct route_assignment_t *assign, GList *nodes) {
  int route_id;
  int fanout;
  int src_id;
  int num_nodes;
  GList *dst_ids, *it, *dst_it;
  struct route_t *tmp_route;
  int *costs;
  int **dijkstra_traceback;
  int *prev_nodes;
  int *reached;
  int i, 
      can_be_static,
#ifdef DBG_DIJKSTRA
      j, 
#endif
      type;

  tmp_route = nodes->data;
  route_id = tmp_route->route_id;
  src_id = tmp_route->prog_id_from;
  fanout = tmp_route->fanout;
  dst_ids = NULL;
  num_nodes = fanout+1;
  can_be_static = 1;
  type = tmp_route->type;

  dijkstra_traceback = calloc(sizeof(int*), num_nodes);
  assert(dijkstra_traceback);

  reached = calloc(sizeof(int), assign->n_row*assign->n_col);
  assert(reached);

  for (it=nodes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->route_id != route_id)
      break;
    assert(tmp_route->prog_id_from == src_id);
    dst_ids = g_list_prepend(dst_ids, &tmp_route->prog_id_to);
  }
  dst_ids = g_list_reverse(dst_ids);

#ifdef DBG_DIJKSTRA
  printf("\tStarting broadcast routing for ID: %d (fanout %d, source %d)\n",
      route_id, fanout, src_id);
#endif
  assert(g_list_length(dst_ids) == fanout);

  costs = calloc(sizeof(int), num_nodes*num_nodes);
  assert(costs);
  dijkstra_traceback[0] = dijkstra_get_branch_costs(assign, src_id, dst_ids, &costs[0]+1, type, route_id);
  costs[0] = 0;
  it = dst_ids;
  for (i=1; i<num_nodes; i++) {
    costs[i*num_nodes] = 0;
    dijkstra_traceback[i] = 
      dijkstra_get_branch_costs(assign, *(int*)it->data, dst_ids, &costs[i*(num_nodes)+1], type, route_id);
    it=it->next;
  }

  for (i=0; i<num_nodes*num_nodes; i++) {
    if (costs[i] < 0) 
      costs[i] = -1;
  }

#ifdef DBG_DIJKSTRA
  it = dst_ids;
  printf("\n%7s:%6d", "From/To", src_id);
  for (it=dst_ids; it; it=it->next)
    printf("%6d", *(int*)it->data);
  printf("\n%7d:", src_id);
  for (i=0; i<num_nodes; i++) {
    if (costs[i] >= 0)
      printf("%6d", costs[i]);
    else
      printf("%6s","");
  }
  it = dst_ids;
  for (j=1; j<num_nodes; j++) {
    printf("\n%7d:", *(int*)it->data);
    for (i=0; i<num_nodes; i++) {
      if (costs[i+j*num_nodes] > 0) 
        printf("%6d", costs[i+j*(num_nodes)]);
      else
        printf("%6s","");
    }
    it=it->next;
  }
  printf("\n");
#endif

  int *hops;
  int *order;
  prev_nodes = approx_arbor(num_nodes, costs, src_id, dst_ids, &hops, &order);

  i=1;
  dst_it = dst_ids;
  for (i=1; i<=fanout; i++) {
    int src_idx, dst_idx, 
#ifdef DBG_DIJKSTRA
        src_prog_id, 
#endif
        traceback_ptr;
    int this_r, this_c;
    struct node_t *node;
    struct mesh_loc_t from, to;
    struct hop_t *hop;
    int *this_traceback;
    tmp_route = g_list_nth_data(nodes, order[i]-1);
    dst_it = g_list_nth(dst_ids, order[i]-1);
    /*if (tmp_route->route_id != route_id)
      break;*/
    assert(tmp_route->route_id == route_id);
    /* The destination node physical location for the next logical destination. */
    assert((node = g_hash_table_lookup(assign->assigned_nodes, &tmp_route->prog_id_to)));
    /* Check that we have the right inverse mapping (should be by default). */
    assert(*(int*)dst_it->data == tmp_route->prog_id_to);
    /* Get the integer location of the destination node in the physical array. */
    traceback_ptr = linearize_node(node->row, node->col, assign->n_col);
    /* We're iterating over each destination in order. */
    dst_idx = order[i];
    src_idx = prev_nodes[dst_idx];

    g_list_free_full(tmp_route->route_hops, free);
    tmp_route->route_hops = NULL;

    /* Now, invert src_idx to get the source node prog_id for this branch. This
     * is not necessarily the prog_id for the original branch. */

    this_traceback = dijkstra_traceback[src_idx];

#ifdef DBG_DIJKSTRA
    if (src_idx == 0)
      src_prog_id = src_id;
    else 
      src_prog_id = *(int*)g_list_nth_data(dst_ids, src_idx-1);
    printf("\tFollowing traceback array (to %d, back to %d)\n",
        tmp_route->prog_id_to,
        src_prog_id);
#endif
    while (this_traceback[traceback_ptr] >= 0) {
      reached[traceback_ptr] = 1;
      this_r = traceback_ptr/assign->n_col;
      this_c = traceback_ptr%assign->n_col;
      to.row = this_r;
      to.col = this_c;
#ifdef DBG_DIJKSTRA
      printf("\t\tTrace back yields to (%d,%d)\t(%c) ",
          this_r, this_c,
          this_traceback[traceback_ptr] == 0 ? 'S' :
          this_traceback[traceback_ptr] == 1 ? 'W' :
          this_traceback[traceback_ptr] == 2 ? 'N' :
          this_traceback[traceback_ptr] == 3 ? 'E' : '?');
#endif
      switch (this_traceback[traceback_ptr]) {
        case 0:
          assert(this_r != 0);
          this_r--;
          break;
        case 2:
          assert(this_r != assign->n_row-1);
          this_r++;
          break;
        case 3:
          assert(this_c != 0);
          this_c--;
          break;
        case 1:
          assert(this_c != assign->n_col-1);
          this_c++;
          break;
        default:
          assert(0);
      }
#ifdef DBG_DIJKSTRA
      int max_static = type == 2 ? assign->max_static : assign->max_static_scalar;
      printf("from (%d,%d)", this_r, this_c);
      if (route_can_be_made_static(&assign->links[linearize_link(this_r, this_c, this_traceback[traceback_ptr], assign->n_col)], route_id, max_static, type != 2, 1))
        printf("\tSTATIC");


#endif
      from.row = this_r;
      from.col = this_c;
      hop = calloc(sizeof(struct hop_t), 1);
      hop->from = from;
      hop->to = to;

#if 0
      assert(route_can_be_made_static(&assign->links[linearize_link(from.row, from.col, 
              this_traceback[traceback_ptr] ^ 2 /*Toggle dir.*/, assign->n_col)],
            route_id, type == 2 ? assign->max_static : assign->max_static_scalar,
            type != 2));
#endif

      hop->route_id = tmp_route->route_id;
      hop->penalty = tmp_route->penalty;

      tmp_route->route_hops = g_list_prepend(tmp_route->route_hops, hop);
      traceback_ptr = linearize_node(this_r, this_c, assign->n_col);
      /* This happens when the node we're coming from has already been reached
       * by a previous iteration (guaranteed to be earlier in the tree, because
       * we sort by the order reached). This means that our from node is 
       * present, and we can break. */
      if (reached[traceback_ptr]) {
        /*g_list_free_full(tmp_route->route_hops, free);
        tmp_route->route_hops = NULL;*/
#ifdef DBG_DIJKSTRA
        printf("\tVISITED\n");
#endif
        break;
      } else {
#ifdef DBG_DIJKSTRA
        printf("\n");
#endif
      }
    }
    /*tmp_route->nhops = g_list_length(tmp_route->route_hops);*/
    tmp_route->nhops = hops[i] % DYNAMIC_STEP;
    if (hops[i] >= DYNAMIC_STEP)
      can_be_static = 0;
    dst_it = dst_it->next;
    update_links(assign->links, tmp_route, 1, assign->n_col);
    //i++;
  }


  for (i=0; i<num_nodes; i++) {
    free(dijkstra_traceback[i]);
  }
  free(reached);
  free(order);
  free(hops);
  free(dijkstra_traceback);
  free(prev_nodes);
  free(costs);
  return can_be_static;
}

void
assign_nn_routes(struct route_assignment_t *assign, routing_func routing) {
  GList          *it;
  int             last_id;
  struct route_t *tmp_route, *tmp_route_last;

  int initial, final;

  assign->unassigned_routes = g_list_sort(assign->unassigned_routes,
      (GCompareFunc)fanout_first);
  hash_seed = fnv_1a(hash_seed);

  initial = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  last_id = -1;
  for (it = assign->unassigned_routes; it; it = it->next) {
    tmp_route = it->data;
    score_route(assign, tmp_route, routing);
    if (last_id >=0 && tmp_route->route_id != last_id) {
      try_assign_static_route(assign, tmp_route_last);
    }
    last_id = tmp_route->route_id;
    tmp_route_last = tmp_route;
  }

#ifndef DBG_DIJKSTRA
  if (!assign->assigned_routes) {
    assign->assigned_routes = assign->unassigned_routes;
    assign->unassigned_routes = NULL;
    return;
  }
#endif

  assign->assigned_routes = g_list_concat(assign->unassigned_routes,
					  assign->assigned_routes);
  assign->unassigned_routes = NULL;

#ifdef DBG_DIJKSTRA
  print_static_dynamic_routes(assign);
#endif
  final = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  assert(initial == final);
  //printf("\tHandled %d routes.\n", initial);

}


void
assign_all_routes(struct route_assignment_t *assign, routing_func routing) {
  GList          *it;
  int             last_id;
  struct route_t *tmp_route, *tmp_route_last;;

  int initial, final;;

  assign->unassigned_routes = g_list_sort(assign->unassigned_routes,
      (GCompareFunc)fanout_first);
  hash_seed = fnv_1a(hash_seed);

  initial = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  last_id = -1;
  for (it = assign->unassigned_routes; it; it = it->next) {
    tmp_route = it->data;
    score_route(assign, tmp_route, routing);
    if (last_id >=0 && tmp_route->route_id != last_id) {
      try_assign_static_route(assign, tmp_route_last);
    }
    last_id = tmp_route->route_id;
    tmp_route_last = tmp_route;
  }

#ifndef DBG_DIJKSTRA
  if (!assign->assigned_routes) {
    assign->assigned_routes = assign->unassigned_routes;
    assign->unassigned_routes = NULL;
    return;
  }
#endif

  assign->assigned_routes = g_list_concat(assign->unassigned_routes,
					  assign->assigned_routes);
  assign->unassigned_routes = NULL;

#ifdef DBG_DIJKSTRA
  print_static_dynamic_routes(assign);
#endif
  final = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  assert(initial == final);
  //printf("\tHandled %d routes.\n", initial);

}

void
assign_all_routes_dijkstra(struct route_assignment_t *assign, routing_func routing) {
  GList          *it;
  int             last_id;
  struct route_t *tmp_route;
  int is_free;

  int initial, final;;

  assign->unassigned_routes = g_list_sort(assign->unassigned_routes,
      (GCompareFunc)fanout_first);
  hash_seed = fnv_1a(hash_seed);

  initial = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  last_id = -1;
  for (it = assign->unassigned_routes; it; it = it->next) {
    tmp_route = it->data;
    is_free = 0;
    if (tmp_route->type == 0)
      is_free = 1;
    if (tmp_route->ag_mc_link && tmp_route->nhops == 0)
      is_free = 1;
    if (tmp_route->argin_link)
      is_free = 1;
    if (!tmp_route->ag_mc_link && !tmp_route->argin_link) {
      if (do_fanout_route(assign, it))
        move_route_to_static(assign, tmp_route->route_id, is_free, tmp_route->type != 2);
      last_id = tmp_route->route_id;
      //try_assign_static_route(assign, tmp_route);
      while (tmp_route->route_id == last_id) {
        it=it->next;
        if (!it)
          break;
        tmp_route = it->data;
      }
      if (!it)
        break;
      it=it->prev;
    } else {
      //printf("Route %d normally\n", tmp_route->route_id);
      tmp_route->is_static = 0;
      score_route(assign, tmp_route, routing);
      try_assign_static_route(assign, tmp_route);
      last_id = tmp_route->route_id;
    }
  }

#ifndef DBG_DIJKSTRA
  if (!assign->assigned_routes) {
    assign->assigned_routes = assign->unassigned_routes;
    assign->unassigned_routes = NULL;
    return;
  }
#endif

  assign->assigned_routes = g_list_concat(assign->unassigned_routes,
					  assign->assigned_routes);
  assign->unassigned_routes = NULL;

#ifdef DBG_DIJKSTRA
  print_static_dynamic_routes(assign);
#endif
  final = g_list_length(assign->unassigned_routes) 
    + g_list_length(assign->assigned_routes);

  assert(initial == final);
  //printf("\tHandled %d routes.\n", initial);

}

void
unassign_node_by_id(struct route_assignment_t *assign, int prog_id) {
  struct node_t  *p_node,
                 *c_node;
  struct route_t *route;
  GList          *l;
  GList          *tmp_routes;
  GHashTable     *route_ids_to_unplace;

  route_ids_to_unplace = g_hash_table_new(g_int_hash, g_int_equal);

  assert((c_node = g_hash_table_lookup(assign->assigned_nodes, &prog_id)));

  /*
   * Now, split the nodes and add them to their respective queues. 
   */
  assert(routing_f);
  assert((p_node = malloc(sizeof(struct node_t))));
  memset(p_node, 0, sizeof(*p_node));
  p_node->prog_id = c_node->prog_id;
  p_node->name = c_node->name;
  p_node->type = c_node->type;
  c_node->name = NULL;

  assign->free_nodes[c_node->type] =
      g_list_prepend(assign->free_nodes[c_node->type], c_node);

  assign->unassigned_nodes =
      g_list_prepend(assign->unassigned_nodes, p_node);

  tmp_routes = NULL;

  /*
   * Now, remove any recently unassigned routes. 
   */
  while ((l = assign->assigned_routes)) {
    route = l->data;

    assign->assigned_routes =
	g_list_remove_link(assign->assigned_routes, l);

    if (route->prog_id_from == p_node->prog_id
	|| route->prog_id_to == p_node->prog_id) {
      unscore_route(assign, route, routing_f);
      g_hash_table_add(route_ids_to_unplace, &route->route_id);
      assign->unassigned_routes =
	  g_list_concat(l, assign->unassigned_routes);
    } else {
      tmp_routes = g_list_concat(l, tmp_routes);
    }
  }

  assert(!assign->assigned_routes);

  while ((l = tmp_routes)) {
    route = l->data;
    tmp_routes = g_list_remove_link(tmp_routes, l);

    if (g_hash_table_contains(route_ids_to_unplace, &route->route_id)) {
      unscore_route(assign, route, routing_f);
      assign->unassigned_routes = g_list_concat(l, assign->unassigned_routes);
    } else {
      assign->assigned_routes = g_list_concat(l, assign->assigned_routes);
    }
  }

  /*
   * Remove the assigned ID after removing routes. 
   */
  assert(g_hash_table_steal(assign->assigned_nodes, &prog_id));
  /*
   * This allows the hash table lookup to succeed for the to-be-deleted
   * ID. 
   */
  c_node->prog_id = -1;

  /*assign->assigned_routes = tmp_routes;*/

  g_hash_table_destroy(route_ids_to_unplace);
}

int
unassign_nodes_directed(struct route_assignment_t *assign,
			struct score_t *score, int count) {
  GHashTableIter  it;
  GList          *l;
  int            *prog_id;
  int             worst_id;
  struct node_t  *node;
  struct route_t *route;
  float           tmp,
                  worst_score;
  int             i;

  if (!g_hash_table_size(assign->assigned_nodes))
    return 0;

  /*
   * Start by looping over every prog_id in the assigned nodes hash
   * table. Set all the unassign scores to 0. 
   */
  g_hash_table_iter_init(&it, assign->assigned_nodes);
  while (g_hash_table_iter_next(&it, (void *) &prog_id, (void *) &node)) {
    node->unassign_score = 0.0f;
  }

  /*
   * For each route, calculate an "unassign score" that includes how
   * much the route contributed to the penalties. Attribute this score
   * to the beginning and ending nodes. 
   */
#if 0
  for (l = assign->assigned_routes; l; l = l->next) {
    route = l->data;
    tmp = 0;
    /*
     * tmp = (*assign->unassign_score)(route, score); 
     */
    node =
	g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_from);
    node->unassign_score += tmp;
    node = g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_to);
    node->unassign_score += tmp;
  }
#endif
  for (l = assign->links[score->worst_link_id].routes; l; l = l->next) {
    route = l->data;
    tmp = 100 * route->penalty;
    node =
	g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_from);
    node->unassign_score += tmp * route->fanout * (route->is_static ? 1 : 10);
    node = g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_to);
    node->unassign_score += tmp* route->fanout * (route->is_static ? 1 : 10);;
  }
  for (l = assign->assigned_routes; l; l = l->next) {
    route = l->data;
    tmp = 1 * route->nhops;
    if (route->nhops == score->longest_route_hops)
      tmp += 1 * score->longest_route_hops;
    node =
	g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_from);
    node->unassign_score += tmp;
    node = g_hash_table_lookup(assign->assigned_nodes, &route->prog_id_to);
    node->unassign_score += tmp;
  }

  /*
   * Store the prog_id with the highest "unassign score" and then call
   * the unassigning function at the end. 
   */
  for (i = 0; i < count; i++) {
    if (!g_hash_table_size(assign->assigned_nodes))
      break;
    worst_id = 0;
    worst_score = -FLT_MAX;
    g_hash_table_iter_init(&it, assign->assigned_nodes);
    while (g_hash_table_iter_next(&it, (void *) &prog_id, (void *) &node)) {
      if (node->unassign_score <= worst_score)
	continue;
      worst_score = node->unassign_score;
      worst_id = *prog_id;
    }
    unassign_node_by_id(assign, worst_id);
  }

  return i;
}

int
unassign_node_random(struct route_assignment_t *assign) {
  int             candidates,
                  selected,
                  i;
  int            *prog_id;
  void           *dummy;
  GHashTableIter  it;

  candidates = g_hash_table_size(assign->assigned_nodes);

  if (!candidates)
    return 0;

  selected = rand() % candidates;

  g_hash_table_iter_init(&it, assign->assigned_nodes);
  for (i = 0; i < selected; i++)
    assert(g_hash_table_iter_next(&it, NULL, NULL));

  /*
   * it now points to the node to unassign. 
   */
  assert(g_hash_table_iter_next(&it, (void *) &prog_id, &dummy));

#ifdef DBG
  printf("Randomly unassigning node: %d (%d of %d)\n", *prog_id,
	 selected, candidates);
#endif

  assert(g_hash_table_size(assign->assigned_nodes) == candidates);
  assert(g_hash_table_lookup(assign->assigned_nodes, prog_id));
  unassign_node_by_id(assign, *prog_id);
  assert(g_hash_table_size(assign->assigned_nodes) == candidates - 1);

  return 1;
}

void
step_route_assignment(struct route_assignment_t *assign,
		      struct score_t *score, routing_func routing,
                      int use_dijkstra) {
  int             type;
  type = rand() % 100;
  clear_static_routes(assign);
  if (type < 20) {
    while (unassign_node_random(assign));
  } else if (type < 40) {
    for (int i=0; i<5; i++)
      unassign_node_random(assign);
  } else {
    type *= 2;
    unassign_nodes_directed(assign, score, type);
  }
  unassign_all_routes(assign, routing);
  type = rand() % 100;
  if (type < 10) {
    while (assign_node_random(assign));
  } else if (type < 30) {
    for (int i=0; i<5; i++)
      if (!assign_node_random(assign))
        break;
    while (assign_node_directed(assign));
  } else {
    while (assign_node_directed(assign));
  }

  //if ((type & 3) && use_dijkstra)
  type = rand() % 100;
  if (type < use_dijkstra)
    assign_all_routes_dijkstra(assign, routing);
  else
    assign_all_routes(assign, routing);
  /*greedy_assign_static_routes(assign);*/
  /*
   * fix_route_deadlocks(assign);
   */

}
