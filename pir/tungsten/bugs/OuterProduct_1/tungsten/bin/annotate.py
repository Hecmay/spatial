import pandas as pd
import argparse
import os
import xml.etree.ElementTree as ET
import pydot
import fnmatch
import json

## With colour library
from colour import Color
red = Color("red")
white = Color("white")
green = Color("green")
orange = Color("orange")
light_green = Color(hex="#afec57")
dark_green = Color(hex="#0e5d2c")
gradient = list(light_green.range_to(dark_green, 101))
# print(red.hex_l)
# print(green.hex_l)
# print([c.hex_l for c in gradient])

## Without colour library
# red = '#ff0000'
# green = '#008000'
# gradient = ['#ff0000', '#fe0500', '#fc0a00', '#fb0f00', '#fa1400', '#f91900', '#f71e00', '#f62200', '#f52700', '#f42c00', '#f23000', '#f13500', '#f03a00', '#ee3e00', '#ed4200', '#ec4700', '#eb4b00', '#e94f00', '#e85400', '#e75800', '#e65c00', '#e46000', '#e36400', '#e26800', '#e16c00', '#df7000', '#de7300', '#dd7700', '#db7b00', '#da7f00', '#d98200', '#d88600', '#d68900', '#d58d00', '#d49000', '#d39300', '#d19700', '#d09a00', '#cf9d00', '#cda000', '#cca300', '#cba600', '#caa900', '#c8ac00', '#c7af00', '#c6b200', '#c5b500', '#c3b800', '#c2ba00', '#c1bd00', '#bfbf00', '#babe00', '#b5bd00', '#b0bc00', '#acba00', '#a7b900', '#a2b800', '#9db700', '#98b500', '#94b400', '#8fb300', '#8ab200', '#86b000', '#81af00', '#7dae00', '#79ac00', '#74ab00', '#70aa00', '#6ca900', '#68a700', '#64a600', '#60a500', '#5ca400', '#58a200', '#54a100', '#50a000', '#4c9e00', '#489d00', '#459c00', '#419b00', '#3d9900', '#3a9800', '#369700', '#339600', '#2f9400', '#2c9300', '#299200', '#269100', '#228f00', '#1f8e00', '#1c8d00', '#198b00', '#168a00', '#138900', '#108800', '#0d8600', '#0b8500', '#088400', '#058300', '#038100', '#008000']

# def xmlns(tag):
    # return '{http://www.w3.org/2000/svg}' + tag 

# def classof(node):
    # if 'class' not in node.attrib:
        # return None
    # return node.attrib['class']

# def title(node):
    # return node.find(xmlns('title')).text

# def annotate_ctx(node, states):
    # if "cluster_Context" not in title(node): return

# def annotate(opts):
    # states = opts.tab.to_dict(orient='index')
    # tree = ET.parse(opts.dot)
    # root = tree.getroot() # svg node
    # for node in root[0].iter(xmlns('g')):
        # annotate_ctx(node, states)
        # if (classof(node) == 'cluster'):
            # print(title(node))

def traverse(graph, func):
    func(graph)
    if (issubclass(type(graph), pydot.Graph)):
        for node in graph.get_node_list():
            traverse(node, func)
        for subgraph in graph.get_subgraph_list():
            traverse(subgraph, func)

def check_cnt(l, states):
    finish_cnt = False
    if ("count=Finite(") in l:
        cnt = int(l.split("count=Finite(")[1].split(")")[0])
        finish_cnt = states['active'] >= cnt
    return finish_cnt

def annotate_default(node, states):
    l = node.get_label().replace("\"","")
    for s in states:
        if s == "inactive": continue
        if s == "cont_inactive": continue
        l += "\n{}={}".format(s,states[s])
    node.set_label(l)

def annotate_ctx(node, states):
    l = node.get_label().replace("\"","")
    finish_cnt = check_cnt(l, states)
    for s in ['active']:
        k = s
        v = states[s]
        if k == 'active': 
            active_pct = float(100*v) / opts.states['cycle']
            v = "{} ({:.2f}%)".format(v, active_pct) 
            if states['complete'] or finish_cnt: v += " [V]"
            else: v += " [X]"
        l += "\n{}={}".format(k,v)
    node.set_label(l)

    if opts.states['deadlock']:
        if states['complete'] or finish_cnt:
            node.set('fillcolor', "\"{}\"".format(green))
        elif not states['ins']:
            node.set('fillcolor', "\"{}\"".format(red))
        elif not states['outs']:
            node.set('fillcolor', "\"{}\"".format(orange))
        else:
            node.set('fillcolor', "\"{}\"".format(green))
    else:
        node.set('fillcolor', "\"{}\"".format(gradient[int(active_pct)]))

def annotate_fifo(node, states):
    l = node.get_label().replace("\"","")
    finish_cnt = check_cnt(l, states)
    def get(key):
        if key not in states: return None
        else: return states[key]
    l += "\nv={} ".format(get('valid'))
    l += "\nr={}".format(get('ready'))
    for s in ['active', 'nelem', 'next']:
        k = s
        if s not in states: continue
        v = states[s]
        if s == 'nelem': v = long(v)
        l += "\n{}={}".format(k,v)
    if opts.states['deadlock']:
        if finish_cnt:
            node.set('fillcolor', "\"{}\"".format(green))
        elif 'valid' in states and not states['valid']:
            node.set('fillcolor', "\"{}\"".format(red))
        elif 'ready' in states and not states['ready']:
            node.set('fillcolor', "\"{}\"".format(orange))
        else:
            node.set('fillcolor', "\"{}\"".format(white))
    node.set_label(l)

def match(patterns, func, node, states):
    if node.annotated: return
    matched = False;
    name = node.get_name().replace("cluster_","")
    for pat in patterns:
        matched = matched or fnmatch.fnmatch(name, pat)
    if matched:
        func(node, states)
        node.annotated = True

def annotate():
    cycle = opts.states['cycle']
    states = opts.states['modules']
    def visitNode(node):
        name = node.get_name().replace("cluster_","")
        if name in states:
            node.annotated = False;
            match(["Context*"], annotate_ctx, node, states[name]);
            match(["BufferRead*", "TokenRead*", "gi*", "go*"], annotate_fifo, node, states[name]);
            match(["*"], annotate_default, node, states[name]);

    graph, = pydot.graph_from_dot_file(opts.dot)
    traverse(graph, visitNode)
    path = opts.dot.replace(".dot", "_sim.html")
    succeed = graph.write(path, format='svg')
    if not succeed:
        print("Write to {} failed!".format(path))
    else:
        print("Annotated graph at {}".format(path))

def main():
    parser = argparse.ArgumentParser(description='Load simulation states')
    parser.add_argument('-s', '--state_path', type=str, default='{}/logs/state.json'.format(os.getcwd()), help='state log path')
    parser.add_argument('-d', '--dot', type=str, default='../pir/out/dot/simple9.dot', 
        help='path to dot graph in dot to annotate on')

    global opts
    (opts, args) = parser.parse_known_args()

    # tab = pd.read_json("file://{}".format(opts.state_path), orient='records')
    # tab = tab.set_index("module")
    # print(tab)
    # opts.tab = tab
    # opt.states = tab.to_dict(orient='index')

    with open(opts.state_path, 'r') as f:
        opts.states = json.load(f)
    annotate()

if __name__ == "__main__":
    main()
