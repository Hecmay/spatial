#include "dot.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

int
dot_lin_row_col(int row, int col, int nrow, int ncol) {
  //return col + (nrow-row-1) * max(ncol, nrow);
  return col + (nrow-row-1) * ncol;
}


void
dot_print_assigned_routes(const char *file,
			  struct route_assignment_t *route) {
  FILE           *out;
  int             i,
                  j;
                  /*s_row,
                  s_col,
                  d_row,
                  d_col*/
  GHashTableIter  it;
  GList          *hops;
  struct node_t  *node;
  struct hop_t   *tmp_hop;
  int            *prog_id;
  out = fopen(file, "w");
  assert(out);

  fprintf(out, "digraph {\n");

  for (i = 0; i < route->n_row; i++) {
    for (j = 0; j < route->n_col; j++) {
      fprintf(out,
	      "r%d_%d \n[label=\"%d\"\nshape=box\npos=\"%d,%d!\"]\n",
	      j, i, dot_lin_row_col(i,j,route->n_row,route->n_col), j * 2, i * 2);
	      /*j, i, j+(route->n_row-i - 1)*route->n_col, j * 2, i * 2);*/
	     /* j, i, i, j, j * 2, i * 2);*/
    }
  }

  /*
   * Start by looping over every prog_id in the assigned nodes hash
   * table. Set all the unassign scores to 0. 
   */
  g_hash_table_iter_init(&it, route->assigned_nodes);
  while (g_hash_table_iter_next(&it, (void *) &prog_id, (void *) &node)) {
    char* color;
    char* stp;
    switch (node->type) {
      case 0 : 
        color = "orange";
        stp = "ArgFringe";
        break;
      case 1 : 
        color = "forestgreen";
        stp = "MC";
        break;
      case 2 : 
        color = "lightseagreen";
        stp = "PMU";
        break;
      case 3 : 
        color = "palevioletred1";
        stp = "DramAG";
        break;
      case 4 : 
        color = "dodgerblue";
        stp = "PCU";
        break;
      default:
        color = "black";
        stp = "UNKNOWN";
        break;

    }
    fprintf(out, "n%d_%d_%d \n[label=\"%d[%s]\"\nshape=ellipse color=%s style=filled pos=\"%d,%d\"]\n",
      node->col, node->row, node->type, node->prog_id, stp, color,
            node->col*2-1, node->row*2-1);
    fprintf(out, "n%d_%d_%d -> r%d_%d\n",
      node->col, node->row, node->type, node->col, node->row);

  }



  /*
   * Iterate over all possible links and add them. 
   */
  for (i = 0; i < route->n_row * route->n_col * 4; i++) {
    /*if (route->links[i].total_weight == 0) {
      assert(route->links[i].routes == NULL);
      continue;
    }
    s_row = (i / 4) / route->n_col;
    s_col = (i / 4) % route->n_col;
    d_row = s_row;
    d_col = s_col;

    assert(i % 4 + 4 * (s_col + s_row * route->n_col) == i);

    switch (i % 4) {
    case 0:
      d_row = s_row - 1;
      break;
    case 1:
      d_col = s_col + 1;
      break;
    case 2:
      d_row = s_row + 1;
      break;
    case 3:
      d_col = s_col - 1;
      break;
    }
    assert(s_col >= 0);
    assert(d_col >= 0);
    assert(s_row >= 0);
    assert(d_row >= 0);
    fprintf(out, "r%d_%d -> r%d_%d [label=\"%d\"]\n",
	    s_col, s_row, d_col, d_row, route->links[i].total_weight);*/
  }

  int xc = route->xc;
  int yc = route->yc;
  /* Iterate over all the assigned program routes, and add them. */
  for (hops = route->merged_hops; hops; hops = hops->next) {
    tmp_hop = hops->data;
    fprintf(out, "r%d_%d -> r%d_%d [label=\"%d V%d\"\npenwidth=%d]\n",
          tmp_hop->from.col*xc, tmp_hop->from.row*yc,
          tmp_hop->to.col*xc,   tmp_hop->to.row*yc,
          tmp_hop->route_id, tmp_hop->vc, (int)(log(tmp_hop->penalty)/log(8.0)+1));
  }
  for (hops = route->merged_hops_static; hops; hops = hops->next) {
    tmp_hop = hops->data;
    fprintf(out, "r%d_%d -> r%d_%d [label=\"%d\"\npenwidth=%d\ncolor=green]\n",
          tmp_hop->from.col, tmp_hop->from.row,
          tmp_hop->to.col,   tmp_hop->to.row,
          tmp_hop->route_id, (int)(log(tmp_hop->penalty)/log(8.0)+1));
  }

  fprintf(out, "}\n");

  fclose(out);
}

void
place_print_assigned_nodes(const char *file,
			   struct route_assignment_t *route) {
  FILE           *out;
  GHashTableIter  it;
  GList          *l, *h;
  struct node_t  *node;
  struct route_t *tmp_route;
  struct hop_t   *tmp_hop;
  int            *prog_id;
  out = fopen(file, "w");
  assert(out);

  g_hash_table_iter_init(&it, route->assigned_nodes);
  while (g_hash_table_iter_next(&it, (void *) &prog_id, (void *) &node)) {
    fprintf(out, "N\t%d\t%d\n", node->prog_id,
        dot_lin_row_col(node->row, node->col, route->n_row, route->n_col));
	    /*node->col + (route->n_row - node->row - 1) * route->n_col);*/
	    /*node->col * route->n_row + (route->n_row - node->row));*/
    for (l=route->assigned_routes; l; l=l->next) {
      tmp_route = l->data;
      if (tmp_route->prog_id_to == node->prog_id) {
        fprintf(out, "H\t%d\t%d\t%c\t%d\n",
            tmp_route->route_id,
            dot_lin_row_col(node->row, node->col, route->n_row, route->n_col),
            'X', tmp_route->ej_vc);
      }
    }
  }

  /*
   * Now print out all the assigned VCs. 
   */
  for (l = route->assigned_routes; l; l = l->next) {
    tmp_route = l->data;
    if (!tmp_route->is_static)
      fprintf(out, "V\t%d\t%d\n", tmp_route->route_id, tmp_route->vc);
    else
      fprintf(out, "S\t%d\t%d\t%d\t%d\n", tmp_route->route_id,
          tmp_route->prog_id_from, tmp_route->prog_id_to, 
          tmp_route->nhops ?  tmp_route->nhops : 1);
  }

  /* 
   * Now print out all the assigned route hops. The format is:
   * H router route dir vc
   * dir is one of [N]orth, [E]ast, [S]outh, [W]est, or e[X]it
   */
  for (l = route->assigned_routes; l; l = l->next) {
    tmp_route = l->data;
    if (tmp_route->is_static)
      continue;
    /* Printf the route "quality" (highest VC used). */
    fprintf(out, "Q\t%d\t%d\n",
        tmp_route->route_id, tmp_route->quality);

    if (!tmp_route->nhops) {
      struct node_t *tmp_node_from, *tmp_node_to;
      tmp_node_from = g_hash_table_lookup(route->assigned_nodes, &tmp_route->prog_id_from);
      tmp_node_to = g_hash_table_lookup(route->assigned_nodes, &tmp_route->prog_id_to);
      assert(tmp_node_from && tmp_node_to);
      /*if (tmp_route->fanout == 1) {
        assert(tmp_node_from->row == tmp_node_to->row);
        assert(tmp_node_from->col == tmp_node_to->col);
      }*/
      /*fprintf(out, "H\t%d\t%d\t%c\t%d\n",tmp_route->route_id, 
          dot_lin_row_col(tmp_node_from->row, tmp_node_from->col, route->n_row, route->n_col),
          'X', tmp_route->ej_vc);*/
    } else {
      tmp_hop = NULL;
      for (h = tmp_route->route_hops; h; h = h->next) {
        tmp_hop = h->data;
#ifndef PRINT_MERGED_HOPS
        fprintf(out, "H\t%d\t%d\t%c\t%d\n",tmp_route->route_id, 
            dot_lin_row_col(tmp_hop->from.row, tmp_hop->from.col, route->n_row, route->n_col),
            route_hop_to_direction(tmp_hop), tmp_hop->vc);
#endif
      }
      /* Print out the exit hop. */
      /*
        fprintf(out, "H\t%d\t%d\t%c\t%d\n",
            tmp_route->route_id,
            dot_lin_row_col(tmp_hop->to.row, tmp_hop->to.col, route->n_row, route->n_col),
            'X', tmp_route->ej_vc);
            */

    }
  }
#ifdef PRINT_MERGED_HOPS
  merge_route_hops(route);
  for (h = route->merged_hops; h; h=h->next) {
    tmp_hop = h->data;
    fprintf(out, "H\t%d\t%d\t%c\t%d\n",
        tmp_hop->route_id,
        dot_lin_row_col(tmp_hop->from.row*route->yc, tmp_hop->from.col*route->xc, route->n_row, route->n_col),
        route_hop_to_direction(tmp_hop), tmp_hop->vc);
  }
#endif


  fclose(out);
}

void
write_summary_line(const char *file, const char *key, int iter,
    struct cand_pool_t *cand, int endp_vcs, int color_vcs, int usec) {

  int first_time;
  FILE *out;
  struct score_t *score;
  first_time = access(file, F_OK) == -1;
  out = fopen(file, "a+");
  if (first_time) {
    fprintf(out, "Key,it,s,EndVC,NetVC,"
                 "Score,TotPkts,"
                 "DynHopsVec,DynHopsScal,StatHopsVec,StatHopsScal,"
                 "LinkLim,InjectLim,EjectLim,LongRoute,Q0Pct,Q0Lim\n");
  }
  score = cand->scores[0];
  fprintf(out, "%s,%d,%.3f,%d,%d,", key, iter, usec/1000.0/1000.0, endp_vcs, color_vcs);
  fprintf(out, "%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%.4f,%.4f,%.4f,%.0f,%.2f,%.4f\n", 
      score->comp,
      score->total_hops, 
      score->total_route_hops-score->total_route_hops_scal,
      score->total_route_hops_scal,
      score->merged_static_hops-score->merged_static_hops_scal,
      score->merged_static_hops_scal,
      /*(int)(score->unmerged_dynamic_hops-score->total_route_hops),
      ((score->unmerged_dynamic_hops-score->total_route_hops)/(score->total_hops)),*/
      score->worst_link_tokens/score->link_penalty_lim,
      score->inject_port_lim/score->link_penalty_lim,
      score->eject_port_lim/score->link_penalty_lim,
      score->longest_route_hops,
      100*score->tot_q0/score->tot_q,
      score->max_non_q0/score->link_penalty_lim);
  fclose(out);
}

void
write_tungsten_file(const char *file, struct route_assignment_t *route, const char *tungsten_prefix) {
  FILE           *out;
  GList          *l, *h;
  struct node_t  *node_from, *node_to, *node;
  struct route_t *tmp_route;
  struct hop_t   *tmp_hop;
  int            *prog_id;
  int             addr_from, addr_to;
  GHashTableIter  it;

  out = fopen(file, "w");
  assert(out);
  int unique_output_counter = 0;
  for (l = route->assigned_routes; l; l=l->next) {
    tmp_route = l->data;
    node_from = g_hash_table_lookup(route->assigned_nodes, &tmp_route->prog_id_from);
    node_to = g_hash_table_lookup(route->assigned_nodes, &tmp_route->prog_id_to);

    addr_from = dot_lin_row_col(node_from->row*route->yc, node_from->col*route->xc, route->n_row, route->n_col);
    addr_to = dot_lin_row_col(node_to->row*route->yc, node_to->col*route->xc, route->n_row, route->n_col);

    if (tmp_route->is_static) {
      tmp_route->out_buf_id = unique_output_counter++;
      fprintf(out, "set %s addr %d\nset %s vc %d\nset %s flow %d\nset %s net static\n",
          tmp_route->name, addr_from,
          tmp_route->name, tmp_route->route_id,
          tmp_route->name, tmp_route->route_id,
          tmp_route->name);
      fprintf(out, "set %s addr %d\nset %s flow %d\nset %s net static\nset %s vc %d\n",
          tmp_route->dest_name, tmp_route->out_buf_id,
          tmp_route->dest_name, tmp_route->route_id,
          tmp_route->dest_name,
          tmp_route->dest_name, tmp_route->route_id);
    } else {
      fprintf(out, "set %s addr %d\nset %s vc %d\nset %s flow %d\nset %s net dynamic\n",
          tmp_route->name, addr_from,
          tmp_route->name, tmp_route->vc,
          tmp_route->name, tmp_route->route_id,
          tmp_route->name);
      fprintf(out, "set %s addr %d\nset %s flow %d\nset %s vc %d\nset %s net dynamic\n",
          tmp_route->dest_name, addr_to,
          tmp_route->dest_name, tmp_route->route_id,
          tmp_route->dest_name, tmp_route->ej_vc,
          tmp_route->dest_name);
    }
  }
  merge_route_hops(route);
  for (h = route->merged_hops; h; h=h->next) {
    tmp_hop = h->data;
    //fprintf(out, "H\t%d\t%d\t%c\t%d\n",
    fprintf(out, "push %s/net/router%d route f%dd%dv%d\n",
        tungsten_prefix,
        dot_lin_row_col(tmp_hop->from.row*route->yc, tmp_hop->from.col*route->xc, route->n_row, route->n_col),
        tmp_hop->route_id,
        route_hop_output_port(tmp_hop),
        tmp_hop->vc);
  }
  for (h = route->merged_hops_static; h; h=h->next) {
    tmp_hop = h->data;
    fprintf(out, "push %s/statnet link f%ds%dd%d\n",
        tungsten_prefix,
        tmp_hop->route_id,
        dot_lin_row_col(tmp_hop->from.row*route->yc, tmp_hop->from.col*route->xc, route->n_row, route->n_col),
        dot_lin_row_col(tmp_hop->to.row*route->yc, tmp_hop->to.col*route->xc, route->n_row, route->n_col));
  }

  g_hash_table_iter_init(&it, route->assigned_nodes);
  while (g_hash_table_iter_next(&it, (void *) &prog_id, (void *) &node)) {
    for (l=route->assigned_routes; l; l=l->next) {
      tmp_route = l->data;
      if (tmp_route->prog_id_to == node->prog_id) {
        if (tmp_route->is_static) {
          fprintf(out, "push %s/statnet output f%ds%dd%d\n",
              tungsten_prefix,
              tmp_route->route_id,
              dot_lin_row_col(node->row*route->yc, node->col*route->xc, route->n_row, route->n_col),
              tmp_route->out_buf_id);
        } else {
            fprintf(out, "push %s/net/router%d route f%dd4v%d\n",
                tungsten_prefix,
                dot_lin_row_col(node->row*route->yc, node->col*route->xc, route->n_row, route->n_col),
                tmp_route->route_id,
                tmp_route->ej_vc);
        }
      }
    }
  }

  fprintf(out, "apply /Top/statnet finalize\n");

  fclose(out);

}
