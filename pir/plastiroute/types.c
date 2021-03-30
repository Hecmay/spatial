#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

void           *
copy_mesh_loc(const void *mesh_loc, void *discard) {
  struct mesh_loc_t *ret;
  ret = (struct mesh_loc_t *) malloc(sizeof(struct mesh_loc_t));
  assert(ret);

  memcpy(ret, mesh_loc, sizeof(struct mesh_loc_t));
  return ret;
}

char
route_hop_to_direction(struct hop_t *hop) {
  if (hop->to.row > hop->from.row)
    return 'N';
  else if (hop->to.row < hop->from.row)
    return 'S';
  else if (hop->to.col > hop->from.col)
    return 'E';
  else if (hop->to.col < hop->from.col)
    return 'W';
  return '?';
}

int
route_hop_output_port(struct hop_t *hop) {
  assert(!((hop->to.row != hop->from.row) && hop->to.col != hop->from.col));
  if (hop->to.row > hop->from.row)
    return 3;
  else if (hop->to.row < hop->from.row)
    return 2;
  else if (hop->to.col > hop->from.col)
    return 0;
  else if (hop->to.col < hop->from.col)
    return 1;
  return -1;
}

void           *
copy_hop(const void *hop, void *discard) {
  struct hop_t   *ret;
  ret = malloc(sizeof(struct hop_t));
  assert(ret);

  memcpy(ret, hop, sizeof(struct hop_t));
  return ret;
}

void           *
copy_route(const void *route, void *discard) {
  struct route_t *ret,
                 *temp;
  temp = (struct route_t *) route;
  ret = malloc(sizeof(struct route_t));
  assert(ret);

  memcpy(ret, route, sizeof(struct route_t));
  ret->interm_nodes =
      g_list_copy_deep(temp->interm_nodes, copy_mesh_loc, NULL);
  ret->route_hops = g_list_copy_deep(temp->route_hops, copy_hop, NULL);

  ret->name = strdup(temp->name);
  ret->dest_name = strdup(temp->dest_name);
  assert(ret->name);
  assert(ret->dest_name);
  return ret;
}

void
free_route(void *route) {
  struct route_t *tmp_route;
  tmp_route = route;
  free(tmp_route->name);
  free(tmp_route->dest_name);
  free(tmp_route);
}

void
free_node(void *node) {
  struct node_t *tmp_node;
  tmp_node = node;
  free(tmp_node->name);
  free(tmp_node);
}

void           *
copy_node(const void *node, void *discard) {
  struct node_t  *ret, *temp;
  ret = malloc(sizeof(struct node_t));
  assert(ret);

  memcpy(ret, node, sizeof(struct node_t));

  temp = (struct node_t *)node;
  if (temp->name) {
    ret->name = strdup(temp->name);
    assert(ret->name);
  }
  return ret;
}

struct route_assignment_t *
alloc_route_assignment(int n_row, int n_col) {
  struct route_assignment_t *ret;

  ret = malloc(sizeof(struct route_assignment_t));
  assert(ret);
  memset(ret, 0, sizeof(*ret));

  ret->assigned_nodes = g_hash_table_new_full(g_int_hash, g_int_equal,
					      NULL, free);
  ret->n_row = n_row;
  ret->n_col = n_col;
  ret->frozen = 0;

  ret->links = calloc(sizeof(struct link_t) * ret->n_row * ret->n_col, 4);
  for (int i=0; i<ret->n_row*ret->n_col*4; i++) {
    ret->links[i].id = i;
    ret->links[i].dir = i%4;
    ret->links[i].col = (i/4)%n_col;
    ret->links[i].row = (i/4)/n_col;
  }

  return ret;
}

struct route_assignment_t *
copy_route_assignment(struct route_assignment_t
		      *route) {
  int             i,
                  idx,
                  vc;
  GHashTableIter  it;
  struct route_assignment_t *ret;
  struct route_t *tmp_route;
  void           *key,
                 *val;
  GList          *l;

  ret = alloc_route_assignment(route->n_row, route->n_col);
  ret->frozen = 0;

  g_hash_table_iter_init(&it, route->assigned_nodes);
  while (g_hash_table_iter_next(&it, &key, &val)) {
    struct node_t  *tmp = copy_node(val, NULL);
    g_hash_table_insert(ret->assigned_nodes, &tmp->prog_id, tmp);
  }
  assert(g_hash_table_size(route->assigned_nodes) ==
	 g_hash_table_size(ret->assigned_nodes));

  for (i = 0; i < NODE_TYPES; i++)
    ret->free_nodes[i] =
	g_list_copy_deep(route->free_nodes[i], copy_node, NULL);
  ret->unassigned_nodes =
      g_list_copy_deep(route->unassigned_nodes, copy_node, NULL);

  ret->assigned_routes =
      g_list_copy_deep(route->assigned_routes, copy_route, NULL);
  ret->unassigned_routes =
      g_list_copy_deep(route->unassigned_routes, copy_route, NULL);

  for (i = 0; i < 4 * ret->n_row * ret->n_col; i++) {
    memcpy(&ret->links[i], &route->links[i], sizeof(struct link_t));
    memcpy(ret->links[i].unique_static, route->links[i].unique_static, 
        sizeof(ret->links[i].unique_static));
    for (vc = 0; vc < NUM_VCS; vc++)
      ret->links[i].vc_weights[vc] = route->links[i].vc_weights[vc];
    ret->links[i].routes = NULL;
    /*printf("Copy link %d\n", i);*/
    for (l = route->links[i].routes; l; l = l->next) {
      tmp_route = l->data;
      /*printf("Copy route %d.\n", tmp_route->route_id);*/
      idx = g_list_index(route->assigned_routes, tmp_route);
      assert(idx >= 0);
      tmp_route = g_list_nth_data(ret->assigned_routes, idx);
      assert(tmp_route);
      ret->links[i].routes =
	  g_list_prepend(ret->links[i].routes, tmp_route);
      assert(ret->links[i].routes);
    }
  }

  ret->max_static = route->max_static;
  ret->max_static_scalar = route->max_static_scalar;
  ret->max_vc = route->max_vc;
  ret->xc = route->xc;
  ret->yc = route->yc;
  ret->max_penalty = route->max_penalty;

  return ret;
}

struct route_assignment_t *
new_route_assignment(struct program_graph_t *prog, 
    struct chip_graph_t *chip) {
  int             i;
  struct route_assignment_t *ret;
  GList *it;
  struct route_t *tmp_route;
  ret = alloc_route_assignment(chip->n_row, chip->n_col);

  for (i = 0; i < NODE_TYPES; i++)
    ret->free_nodes[i] =
	g_list_copy_deep(chip->free_nodes[i], copy_node, NULL);
  ret->unassigned_nodes =
      g_list_copy_deep(prog->unassigned_nodes, copy_node, NULL);

  ret->unassigned_routes =
      g_list_copy_deep(prog->unassigned_routes, copy_route, NULL);

  ret->max_static = chip->max_static;
  ret->max_static_scalar = chip->max_static_scalar;
  ret->max_vc = chip->max_vc;
  ret->xc = chip->xc;
  ret->yc = chip->yc;
  ret->max_penalty = 0;
  for (it=ret->unassigned_routes; it; it=it->next) {
    tmp_route = it->data;
    if (tmp_route->penalty > ret->max_penalty)
      ret->max_penalty = tmp_route->penalty;
  }
  return ret;
}

void
free_route_assignment(struct route_assignment_t *rt) {
  int             i;
  GList          *it;
  struct route_t *cur;

  for (i = 0; i < 4 * rt->n_row * rt->n_col; i++) {
    g_list_free(rt->links[i].routes);
  }

  free(rt->links);

  g_hash_table_destroy(rt->assigned_nodes);

  for (i = 0; i < NODE_TYPES; i++)
    g_list_free_full(rt->free_nodes[i], free);


  for (it = rt->assigned_routes; it != NULL; it = it->next) {
    cur = (struct route_t *) it->data;
    g_list_free_full(cur->interm_nodes, free);
    g_list_free_full(cur->route_hops, free);
  }
  g_list_free(rt->merged_hops);
  g_list_free(rt->merged_hops_static);

  g_list_free_full(rt->unassigned_nodes, free_node);
  g_list_free_full(rt->assigned_routes, free_route);
  g_list_free_full(rt->unassigned_routes, free_route);
  free(rt);
}

int
compare_node_id(const void *node, const void *data) {
  return (((struct node_t *) node)->prog_id != *(int *) data);
}

GList          *
add_program_node(GList * nodes, int node_id, int type, const char *name) {
  struct node_t  *node;
  node = malloc(sizeof(struct node_t));
  assert(node);

  node->row = node->col = -1;
  node->type = type;
  node->prog_id = node_id;
  node->name = strdup(name);

  return g_list_prepend(nodes, node);
}

GList          *
add_program_route(int route_id, int ctx_id, GList * routes, int src_id, 
                  int dst_id, int out_id, int weight, int type, int ag_mc, int argin,
                  const char *name, const char *dest_name) {
  struct route_t *tmp_route;
  assert((tmp_route = malloc(sizeof(struct route_t))));

  tmp_route->route_id = route_id;
  tmp_route->ctx_id = ctx_id;
  tmp_route->prog_id_from = src_id;
  tmp_route->prog_id_to = dst_id;
  tmp_route->output_id = out_id;
  tmp_route->penalty = weight;
  tmp_route->type = type;
  tmp_route->ag_mc_link = ag_mc;
  tmp_route->argin_link = argin;
  tmp_route->is_static = 0;
  tmp_route->name = strdup(name);
  tmp_route->dest_name = strdup(dest_name);

  /*
   * for valient routing 
   */
  tmp_route->interm_nodes = NULL;

  /*
   * explicit intermediate hops 
   */
  tmp_route->route_hops = NULL;

  return g_list_prepend(routes, tmp_route);
}

GList          *
add_chip_node(GList * nodes, int row, int col, int type) {
  struct node_t  *tmp;
  tmp = malloc(sizeof(struct node_t));
  assert(tmp);

  tmp->row = row;
  tmp->col = col;
  tmp->type = type;
  tmp->prog_id = -1;
  tmp->name = NULL;

  return g_list_prepend(nodes, tmp);
}

int
link_is_ag_mc(struct program_graph_t *assign, int poss_ag, int poss_mc) {
  struct node_t *tmp_node;
  GList *it;
  tmp_node = NULL;
  for (it=assign->unassigned_nodes; it; it=it->next) {
    tmp_node = it->data;
    if (tmp_node->prog_id == poss_ag)
      break;
  }
  assert(tmp_node->prog_id == poss_ag);
  if (tmp_node->type != 3)
    return 0;
  for (it=assign->unassigned_nodes; it; it=it->next) {
    tmp_node = it->data;
    if (tmp_node->prog_id == poss_mc)
      break;
  }
  assert(tmp_node->prog_id == poss_mc);
  if (tmp_node->type != 1)
    return 0;
  return 1;
}

int
link_is_argin(struct program_graph_t *assign, int poss_argin) {
  struct node_t *tmp_node;
  GList *it;
  tmp_node = NULL;
  for (it=assign->unassigned_nodes; it; it=it->next) {
    tmp_node = it->data;
    if (tmp_node->prog_id == poss_argin)
      break;
  }
  assert(tmp_node->prog_id == poss_argin);
  if (tmp_node->type != 0)
    return 0;
  return 1;
}

int
get_uuid(GHashTable *h, const char *s, int *n, int verify) {
  int t;
  char *new_s;

  printf("Request string: (%s)\t", s);

  /* If s in h return value. */
  if ((t = (long int)g_hash_table_lookup(h, s))) {
    if (verify <0) {
      printf("String %s found, should not be present\n", s);
      assert(0);
    }
    printf("%d\n", t);
    return t;
  }
  if (verify>0) {
    printf("String %s not found, should be present\n", s);
    assert(0);
  }
  /* Else, insert n into h and return n++. */
  new_s = strdup(s);
  g_hash_table_insert(h, new_s, (void *)((long int) *n));
  printf("%d+\n", *n);
  return (*n)++;
}

struct dst_spec_t {
  int dst_id;
  int out_id;
};

struct src_spec_t {
  int route_id;
  int ctx_id;
  int src_id;
  int type;
  int weight;
};

int 
find_dst(GList *list, int out_id) {
  struct dst_spec_t *tmp_dst;
  for (; list; list=list->next) {
    tmp_dst = list->data;
    if (tmp_dst->out_id == out_id)
      return tmp_dst->dst_id;
  }
  assert(0);
}

struct src_spec_t *
find_src(GList *list, int route_id) {
  struct src_spec_t *tmp_src;
  for (; list; list=list->next) {
    tmp_src = list->data;
    if (tmp_src->route_id == route_id)
      return tmp_src;
  }
  assert(0);
}

struct program_graph_t *
load_program(const char *fn, const char *fl, const char *fs, const char *fd) {
  FILE           *in_n,
                 *in_l,
                 *in_s,
                 *in_d;
  int             route_id,
                  ctx_id,
                  src_id,
                  dst_id,
                  out_id,
                  weight,
                  id,
                  type;
  struct program_graph_t *ret;
  char            ch;
  int             status;
  struct route_t *tmp_route;
  GList   *it;
  int last_dst;

  GList *dst_maps, *src_maps;
  dst_maps = src_maps = NULL;

  int next_id;
  char node_id_str[256], route_id_str[256], ctx_id_str[256], 
       src_id_str[256], dst_id_str[256], out_id_str[256];
  GHashTable *str_map;

  next_id = 1;
  str_map = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);

  ret = malloc(sizeof(struct program_graph_t));
  assert(ret);
  memset(ret, 0, sizeof(*ret));

  in_n = fopen(fn, "r");
  if (!in_n) {
    fprintf(stderr, "Could not open node file: %s\n", fn);
    assert(0);
  }

  in_l = fopen(fl, "r");
  if (!in_l) {
    fprintf(stderr, "Could not open link file: %s\n", fl);
    assert(0);
  }

  in_s = fopen(fs, "r");
  if (!in_s) {
    fprintf(stderr, "Could not open source file: %s\n", fs);
    assert(0);
  }

  in_d = fopen(fd, "r");
  if (!in_d) {
    fprintf(stderr, "Could not open dest file: %s\n", fd);
    assert(0);
  }

  /*
   * Skip the first line. 
   */
  status = fscanf(in_n, "%*s");
  while ((status = fscanf(in_n, " %255[^,],%d", node_id_str, &type)) == 2) {
    id = get_uuid(str_map, node_id_str, &next_id, -1);
    ret->unassigned_nodes =
	add_program_node(ret->unassigned_nodes, id, type, strdup(node_id_str));
  }
  ret->unassigned_nodes = g_list_reverse(ret->unassigned_nodes);

  status = fscanf(in_d, "%*s");
  while ((status = fscanf(in_d, " %255[^,],%255[^,\n]", dst_id_str, node_id_str)) == 2) {
    dst_id = get_uuid(str_map, node_id_str, &next_id, 1);
    out_id = get_uuid(str_map, dst_id_str, &next_id, -1);

    struct dst_spec_t *tmp;
    tmp = malloc(sizeof(struct dst_spec_t));
    assert(tmp);

    tmp->dst_id = dst_id;
    tmp->out_id = out_id;

    dst_maps = g_list_append(dst_maps, tmp);
  }

  status = fscanf(in_s, "%*s");
  while ((status = fscanf(in_s, " %255[^,],%255[^,],%255[^,],%d,%d", route_id_str, ctx_id_str, src_id_str,&type,&weight)) == 5) {
    route_id = get_uuid(str_map, route_id_str, &next_id, -1);
    ctx_id   = get_uuid(str_map, ctx_id_str,   &next_id, 0);
    src_id   = get_uuid(str_map, src_id_str,   &next_id, 1);

    struct src_spec_t *tmp;
    tmp = malloc(sizeof(struct src_spec_t));
    assert(tmp);

    tmp->route_id = route_id;
    tmp->ctx_id = ctx_id;
    tmp->src_id = src_id;
    tmp->type = type;
    tmp->weight = weight;

    src_maps = g_list_append(src_maps, tmp);
  }

  /*
   * Skip the first line. 
   */
  status = fscanf(in_l, "%*s");
  while (2 == fscanf(in_l, " %255[^,],%255[^,\n]",
		     route_id_str, out_id_str)) {

    struct src_spec_t *src_spec;

    route_id = get_uuid(str_map, route_id_str, &next_id, 1);
    out_id   = get_uuid(str_map, out_id_str,   &next_id, 1);

    dst_id   = find_dst(dst_maps, out_id);

    src_spec = find_src(src_maps, route_id);
    ctx_id   = src_spec->ctx_id;
    src_id   = src_spec->src_id;
    type     = src_spec->type;
    weight   = src_spec->weight;

    int ag_mc, argin;
    int fanout = 1;
    ag_mc = link_is_ag_mc(ret, src_id, dst_id);
#ifdef FREE_ARGIN
    argin = link_is_argin(ret, src_id);
#else
    argin = 0;
#endif
    ret->unassigned_routes = add_program_route(route_id,
                                               ctx_id,
					       ret->unassigned_routes,
					       src_id, dst_id, out_id, weight, type, ag_mc, argin, strdup(route_id_str), strdup(out_id_str));
    last_dst = dst_id;

    /*
     * Read any additional destinations and add extra routes (for
     * now). 
     */
    while (1) {
      ch = fgetc(in_l);
      if (ch == '\n' || ch == '\r')
	break;
      if (ch == ',')
	continue;
      ungetc(ch, in_l);
      if (1 != fscanf(in_l, "%255[^,\n]", out_id_str))
	break;
      out_id   = get_uuid(str_map, out_id_str,   &next_id, 1);
      dst_id   = find_dst(dst_maps, out_id);
      printf("\t Additional dest: %d/%d\n", dst_id, out_id);
      if (dst_id == last_dst)
        continue;
      last_dst = dst_id;
      ag_mc = link_is_ag_mc(ret, src_id, dst_id);
      ret->unassigned_routes =
	  add_program_route(route_id, ctx_id, ret->unassigned_routes, src_id,
			    dst_id, out_id, weight, type, ag_mc, argin, strdup(route_id_str), strdup(out_id_str));
      fanout++;
    }
    for (it=ret->unassigned_routes; it; it=it->next) {
      tmp_route = it->data;
      if (tmp_route->route_id == route_id)
        tmp_route->fanout = fanout;
    }
  }
  ret->unassigned_routes = g_list_reverse(ret->unassigned_routes);

  fclose(in_n);
  fclose(in_l);

  return ret;
}

void
free_program(struct program_graph_t *prog) {
  g_list_free_full(prog->unassigned_nodes, free);
  g_list_free_full(prog->unassigned_routes, free);
  free(prog);
}

void
print_chip_graph(struct chip_graph_t *chip) {
  GList          *it;
  struct node_t  *node;
  int             i;

  printf("Printing chip graph:\n");
  for (i = 0; i < NODE_TYPES; i++) {
    for (it = chip->free_nodes[i]; it; it = it->next) {
      node = it->data;
      printf("\t %d (%d, %d)\n", node->type, node->row, node->col);
    }
  }
  printf("\n");
}

void
print_prog_graph(struct program_graph_t *prog) {
  GList          *it;
  struct node_t  *node;
  struct route_t *route;

  printf("Program nodes:\n");
  for (it = prog->unassigned_nodes; it; it = it->next) {
    node = it->data;
    printf("\t%d (%d)\n", node->prog_id, node->type);
  }

  printf("Program routes:\n");
  for (it = prog->unassigned_routes; it; it = it->next) {
    route = it->data;
    printf("\t%4d: %d -> %d (penalty %d)\n", route->route_id,
	   route->prog_id_from, route->prog_id_to, route->penalty);
  }
  printf("\n");
}

struct chip_graph_t *
new_chip(int nrows, int ncols, int max_static, int max_static_scalar, int xc, int yc, int max_vc, const char* pattern, int pcu_pmu_ratio, int AGdup) {
  struct chip_graph_t *ret;
  int             i,
                  j,
                  type;

  ret = malloc(sizeof(struct chip_graph_t));
  assert(ret);
  memset(ret, 0, sizeof(*ret));

  ret->max_static_scalar = max_static_scalar;
  ret->n_row = nrows;
#ifdef SPLIT_AG_MC
  assert(false);
#else
  ret->n_col = ncols + 2;
#endif
  ret->xc = xc;
  ret->yc = yc;
  ret->max_vc = max_vc;
  assert(ret->n_col % ret->xc == 0);
  assert(ret->n_row % ret->yc == 0);

  /*
   * Add CU array with pattern. 
   *
   *  checkerboard
   *  +-----+-----+
   *  | PCU | PMU |
   *  +-----+-----+
   *  | PMU | PCU |
   *  +-----+-----+
   *
   *  mcmcstrip
   *
   *  +-----+-----+-----+
   *  | PMU | PCU | PMU |
   *  +-----+-----+-----+
   *  | PMU | PCU | PMU |
   *  +-----+-----+-----+
   */
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      if (strcmp(pattern, "checkerboard") == 0) {
        type = (i + j) % (pcu_pmu_ratio+1) ? 4 : 2;
      } else if (strcmp(pattern, "lines") == 0) {
        type = (j) % (pcu_pmu_ratio+1) ? 4 : 2;
      } else if (strcmp(pattern, "mcmcstrip") == 0) { // Re
        assert(pcu_pmu_ratio == 1);
        type = (j % 3 % 2) ? 4 : 2;
      } else {
        fprintf(stderr, "Do not recognize pattern: %s\n", pattern);
        type = 0;
        exit(-1);
      }
      ret->free_nodes[type] = add_chip_node(ret->free_nodes[type], i, j+1, type);
      /*if (i != nrows-1 && j != ncols/2)*/
    }
  }

  /*
   * Add DRAM fringe/address generators. 
   */
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < AGdup; j++) {
      ret->free_nodes[3] = add_chip_node(ret->free_nodes[3], i, 0, 3);
      ret->free_nodes[3] = add_chip_node(ret->free_nodes[3], i, ncols + 1, 3);
      ret->free_nodes[1] = add_chip_node(ret->free_nodes[1], i, 0, 1);
      ret->free_nodes[1] = add_chip_node(ret->free_nodes[1], i, ncols + 1, 1);
    }
  }

  ret->free_nodes[0] =
      add_chip_node(ret->free_nodes[0], nrows - 1, ncols / 2, 0);

  for (i = 0; i < NODE_TYPES; i++)
    ret->free_nodes[i] = g_list_reverse(ret->free_nodes[i]);

  ret->max_static = max_static;
  return ret;
}

void
free_chip(struct chip_graph_t *chip) {
  int             i;

  for (i = 0; i < NODE_TYPES; i++)
    g_list_free_full(chip->free_nodes[i], free);
  free(chip);
}
