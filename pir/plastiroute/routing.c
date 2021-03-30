#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "routing.h"
#include "score.h"
#include "move.h"

/*
 * forward declarations 
 */
extern int      get_buffer_direction(struct mesh_loc_t *from,
				     struct mesh_loc_t *to);
//extern inline int linearize_link(int row, int col, int dir, int ncol);

/*
 * computes the weight of a route. This can be used in comparing two
 * routes. 3 weights are included in the total weight/score of the route:
 * - max link weight: the worst single link total weight resulting from
 * this route - route length - sum of all link weights along the route 
 */
static int
get_route_weight(struct link_t *links, GList * route_hops, int num_cols,
    struct route_t *route) {
  int             w_max_link,
                  w_route_length,
                  w_sum_links,
                  w_not_static;
  int             max_link,
                  sum_links,
                  route_length;
  GList          *it;
  struct hop_t   *hop;
  struct link_t  *link;
  int             ret;
  int             dir;
  int             can_static = 1;

  assert(route_hops);

  /*
   * PARAMETERS can change these parameters to change how routes are
   * scored 
   */
  w_max_link = 100;		/* bias for max link weight */
  w_route_length = 10;		/* bias for route length/number of hops */
  w_sum_links = 0;		/* bias for sum of all link weights */
  w_not_static = 1000;	  /* bias for non-static routes*/
  /*
   * END OF PARAMETERS 
   */

  max_link = 0;
  sum_links = 0;
  route_length = g_list_length(route_hops);

  ret = 0;

  for (it = route_hops; it != NULL; it = it->next) {
    hop = (struct hop_t *) it->data;
    dir = get_buffer_direction(&hop->from, &hop->to);
    link =
	&links[linearize_link
	       (hop->from.row, hop->from.col, dir, num_cols)];
    sum_links += link->total_weight;
    if (link->total_weight > max_link) {
      max_link = link->total_weight;
    }
    if (route->type == 2 && !route_can_be_made_static(link, route->route_id, 1, 0, 1))
      can_static = 0;
  }

  ret =
      w_max_link * max_link + w_route_length * route_length +
      w_sum_links * sum_links;
  if (!can_static)
    ret += w_not_static;
  return ret;
}

static struct mesh_loc_t
get_next_node_loc(struct mesh_loc_t source, int dir) {
  struct mesh_loc_t next;

  assert((dir >= 0) && (dir < 4));

  next.row = source.row;
  next.col = source.col;

  if (dir == 0) {		/* going North */
    next.row = source.row - 1;
  } else if (dir == 2) {	/* going South */
    next.row = source.row + 1;
  } else if (dir == 1) {	/* going East */
    next.col = source.col + 1;
  } else {			/* going West */
    next.col = source.col - 1;
  }
  return next;
}

static int
get_man_dist(struct mesh_loc_t src, struct mesh_loc_t dest) {
  int             dx,
                  dy;

  dx = (src.col >= dest.col) ? (src.col - dest.col) : (dest.col - src.col);
  dy = (src.row >= dest.row) ? (src.row - dest.row) : (dest.row - src.row);
  return dx + dy;
}

int
route_has_duplicates(GList * route_hops) {
  struct hop_t   *last,
                 *current;
  int             ret;
  GList          *it;

  ret = 0;
  it = NULL;
  last = NULL;
  current = NULL;

  for (it = route_hops; it != NULL; it = it->next) {
    current = (struct hop_t *) it->data;
    if (last) {
      if (((current->from.row == last->from.row)
	   && (current->from.col == last->from.col))
	  && ((current->to.row == last->to.row)
	      && (current->to.col == last->to.col))
	  ) {
	ret = 1;
      }
    }
    last = current;
  }
  return ret;
}

GList          *
route_dor_XY(struct link_t * links,
	     struct mesh_loc_t from,
	     struct mesh_loc_t to, int num_cols, int num_rows,
             struct route_t *route) {
  GList          *ret;
  struct hop_t   *hop;

  ret = NULL;

  if (from.col > to.col) {
    for (; from.col != to.col; from.col--) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.row = from.row;
      hop->to.row = from.row;
      hop->from.col = from.col;
      hop->to.col = from.col - 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  } else if (from.col < to.col) {
    for (; from.col != to.col; from.col++) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.row = from.row;
      hop->to.row = from.row;
      hop->from.col = from.col;
      hop->to.col = from.col + 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  }

  assert(from.col == to.col);

  if (from.row > to.row) {
    for (; from.row != to.row; from.row--) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.col = to.col;
      hop->to.col = to.col;
      hop->from.row = from.row;
      hop->to.row = from.row - 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  } else if (from.row < to.row) {
    for (; from.row != to.row; from.row++) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.col = to.col;
      hop->to.col = to.col;
      hop->from.row = from.row;
      hop->to.row = from.row + 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  }

  assert(from.row == to.row);
  assert(route_has_duplicates(ret) == 0);

  return ret;
}

GList          *
route_dor_YX(struct link_t * links,
	     struct mesh_loc_t from,
	     struct mesh_loc_t to, int num_cols, int num_rows,
             struct route_t *route) {
  GList          *ret;
  struct hop_t   *hop;

  ret = NULL;

  if (from.row > to.row) {
    for (; from.row != to.row; from.row--) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.col = from.col;
      hop->to.col = from.col;
      hop->from.row = from.row;
      hop->to.row = from.row - 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  } else if (from.row < to.row) {
    for (; from.row != to.row; from.row++) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.col = from.col;
      hop->to.col = from.col;
      hop->from.row = from.row;
      hop->to.row = from.row + 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  }

  assert(from.row == to.row);

  if (from.col > to.col) {
    for (; from.col != to.col; from.col--) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.row = to.row;
      hop->to.row = to.row;
      hop->from.col = from.col;
      hop->to.col = from.col - 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  } else if (from.col < to.col) {
    for (; from.col != to.col; from.col++) {
      hop = malloc(sizeof(struct hop_t));
      assert(hop);
      hop->from.row = to.row;
      hop->to.row = to.row;
      hop->from.col = from.col;
      hop->to.col = from.col + 1;
      hop->penalty = route->penalty;
      ret = g_list_append(ret, hop);
    }
  }

  assert(from.col == to.col);
  assert(route_has_duplicates(ret) == 0);

  return ret;
}

GList          *
route_dor(struct link_t * links,
	  struct mesh_loc_t from,
	  struct mesh_loc_t to, int num_cols, int num_rows,
          struct route_t *route) {
  return route_dor_XY(links, from, to, num_cols, num_rows, route);
}

GList          *
route_bdor(struct link_t * links,
	   struct mesh_loc_t from,
	   struct mesh_loc_t to, int num_cols, int num_rows, 
           struct route_t *route) {
  GList          *ret;
  int             diff_x,
                  diff_y;
  routing_func    rfunc;

  ret = NULL;
  diff_x =
      (from.col >= to.col) ? (from.col - to.col) : (to.col - from.col);
  diff_y =
      (from.row >= to.row) ? (from.row - to.row) : (to.row - from.row);

  if (diff_x == 0) {
    rfunc = route_dor_XY;
  } else if (diff_y == 0) {
    rfunc = route_dor_YX;
  } else if (diff_x > diff_y) {
    rfunc = route_dor_YX;
  } else if (diff_x < diff_y) {
    rfunc = route_dor_XY;
  } else {
    rfunc = ((diff_x % 2) == 0) ? route_dor_XY : route_dor_YX;
  }

  ret = rfunc(links, from, to, num_cols, num_rows, route);
  assert(route_has_duplicates(ret) == 0);
  return ret;
}

GList          *
route_min_local(struct link_t * links,
		struct mesh_loc_t from,
		struct mesh_loc_t to, int num_cols, int num_rows,
                struct route_t *route) {
  GList          *ret;
  int             dir_x,
                  dir_y;
  struct mesh_loc_t cur_loc,
                  next_x,
                  next_y,
                  next;
  int             dist_x,
                  dist_y;
  struct hop_t   *hop;
  struct link_t  *link_x,
                 *link_y;

  ret = NULL;

  dir_x = (to.col >= from.col) ? 1 : 3;
  dir_y = (to.row >= from.row) ? 2 : 0;
  cur_loc = from;

  while (!((cur_loc.row == to.row) && (cur_loc.col == to.col))) {
    next_x = get_next_node_loc(cur_loc, dir_x);
    next_y = get_next_node_loc(cur_loc, dir_y);

    dist_x = get_man_dist(next_x, to);
    dist_y = get_man_dist(next_y, to);

    if (dist_x == dist_y) {
      link_x =
	  &links[linearize_link
		 (cur_loc.row, cur_loc.col, dir_x, num_cols)];
      link_y =
	  &links[linearize_link
		 (cur_loc.row, cur_loc.col, dir_y, num_cols)];
      if (link_x->total_weight <= link_y->total_weight) {
	next = next_x;
      } else {
	next = next_y;
      }
    } else if (dist_x < dist_y) {
      next = next_x;
    } else {
      next = next_y;
    }

    hop = (struct hop_t *) malloc(sizeof(struct hop_t));
    assert(hop);
    hop->from = cur_loc;
    hop->to = next;
    ret = g_list_append(ret, hop);
    cur_loc = next;
  }
  assert(ret && "didn't do a good");
  assert(route_has_duplicates(ret) == 0);
  return ret;
}

GList          *
route_valient(struct link_t * links,
	      struct mesh_loc_t from,
	      struct mesh_loc_t to, int num_cols, int num_rows,
              struct route_t *route) {
  struct mesh_loc_t interm_node;
  GList          *ret;

  ret = NULL;

  interm_node.row = rand() % num_rows;
  interm_node.col = rand() % num_cols;

  if ((interm_node.row == from.row) && (interm_node.col == from.col)) {
    interm_node.row = from.row;
    interm_node.col = from.col;
  } else if ((interm_node.row == to.row) && (interm_node.col == to.col)) {
    interm_node.row = from.row;
    interm_node.col = from.col;
  } else {
    ret = route_dor(links, from, interm_node, num_cols, num_rows, route);
  }
  ret =
      g_list_concat(ret,
		    route_dor(links, interm_node, to, num_cols, num_rows, route));

  return ret;
}

GList          *
route_min_valient(struct link_t * links,
		  struct mesh_loc_t from,
		  struct mesh_loc_t to, int num_cols, int num_rows,
                  struct route_t *route) {
  GList          *ret;
  int             dx,
                  dy;
  struct mesh_loc_t interm_node;

  ret = NULL;

  dx = (from.col >= to.col) ? (from.col - to.col) : (to.col - from.col);
  dy = (from.row >= to.row) ? (from.row - to.row) : (to.row - from.row);

  interm_node.row =
      (to.row >=
       from.row) ? (from.row + (rand() % (dy + 1))) : (from.row -
						       (rand() %
							(dy + 1)));
  interm_node.col =
      (to.col >=
       from.col) ? (from.col + (rand() % (dx + 1))) : (from.col -
						       (rand() %
							(dx + 1)));

  if ((interm_node.col == from.col) && (interm_node.row == from.col)) {
    ret = route_dor(links, from, to, num_cols, num_rows, route);
  } else if ((interm_node.col == to.col) && (interm_node.row == to.row)) {
    ret = route_dor(links, from, to, num_cols, num_rows, route);
  } else {
    ret = route_dor(links, from, interm_node, num_cols, num_rows, route);
    ret =
	g_list_concat(ret,
		      route_dor(links, interm_node, to, num_cols,
				num_rows, route));
  }

  return ret;
}

GList          *
route_directed_valient(struct link_t * links,
		       struct mesh_loc_t from,
		       struct mesh_loc_t to, int num_cols, int num_rows,
                       struct route_t *route) {
  GList          *ret;
  GList          *temp_route;
  int             cur_best_score;
  int             weight;
  int             i,
                  j;
  struct mesh_loc_t interm_node;

  ret = NULL;
  temp_route = NULL;
  cur_best_score = INT_MAX;

  for (i = 0; i < num_cols; i++) {
    for (j = 0; j < num_rows; j++) {
      if (((i == from.col) && (j == from.row))
	  || ((i == to.col) && (j == to.row))) {
	continue;
      }
      interm_node.col = i;
      interm_node.row = j;
      temp_route = route_dor(links, from, interm_node, num_cols, num_rows, route);
      temp_route =
	  g_list_concat(temp_route,
			route_dor(links, interm_node, to, num_cols,
				  num_rows, route));
      weight = get_route_weight(links, temp_route, num_cols, route);
      if (weight < cur_best_score) {
	cur_best_score = weight;
	g_list_free_full(ret, free);
	ret = temp_route;
      } else {
	g_list_free_full(temp_route, free);
      }
    }
  }
  return ret;
}

GList          *
route_min_directed_valient(struct link_t * links,
		       struct mesh_loc_t from,
		       struct mesh_loc_t to, int num_cols, int num_rows,
                       struct route_t *route) {
  GList          *ret;
  GList          *temp_route_h1, *temp_route_h2, *temp_route;
  int             cur_best_score;
  int             weight;
  int             i,
                  j,
                  k;
  int min_col, max_col, min_row, max_row;
  struct mesh_loc_t interm_node;

  ret = NULL;
  temp_route = NULL;
  cur_best_score = INT_MAX;

  min_col = from.col < to.col ? from.col : to.col;
  max_col = from.col > to.col ? from.col : to.col;
  min_row = from.row < to.row ? from.row : to.row;
  max_row = from.row > to.row ? from.row : to.row;

  assert(min_col >= 0);
  assert(min_row >= 0);
  assert(max_col < num_cols);
  assert(max_row < num_rows);
  
  if (min_col == max_col || min_row == max_row)
    return route_dor(links, from, to, num_cols, num_rows, route);

  for (i = min_col; i <=max_col ; i++) {
    for (j = min_row; j <= max_row; j++) {
      for (k = 0; k < 4; k++) {
        if (((i == from.col) && (j == from.row))
            || ((i == to.col) && (j == to.row))) {
          continue;
        }
        interm_node.col = i;
        interm_node.row = j;
        if (k & 1) {
          temp_route_h1 = route_dor(links, from, interm_node, num_cols, num_rows, route);
        } else {
          temp_route_h1 = route_dor_YX(links, from, interm_node, num_cols, num_rows, route);
        }
        if (k & 2) {
          temp_route_h2 = route_dor(links, interm_node, to, num_cols, num_rows, route);
        } else {
          temp_route_h2 = route_dor_YX(links, interm_node, to, num_cols, num_rows, route);
        }
        temp_route = g_list_concat(temp_route_h1, temp_route_h2);
        weight = get_route_weight(links, temp_route, num_cols, route);
        if (weight < cur_best_score) {
          cur_best_score = weight;
          g_list_free_full(ret, free);
          ret = temp_route;
        } else {
          g_list_free_full(temp_route, free);
        }
      }
    }
  }
  return ret;
}
