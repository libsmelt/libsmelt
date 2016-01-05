# Copyright (c) 2007, 2008, 2009, 2010, 2011, 2012, 2013 ETH Zurich.

# Import graphviz
import sys
import os
sys.path.append('..')
sys.path.append('/usr/lib/graphviz/python/')
sys.path.append('/usr/lib64/graphviz/python/')
sys.path.append('/home/skaestle/bin/')
sys.path.append(os.getenv('HOME') + '/bin/')
import gv
import Queue
import numpy
import subprocess
import logging
import pdb
import traceback
import re
import general

from config import topologies, machines, get_ab_machine_results, get_machine_result_suffix
from general import _pgf_header, \
    _pgf_plot_header, \
    _pgf_plot_footer, \
    _pgf_footer, \
    do_pgf_plot, \
    do_pgf_3d_plot, \
    do_pgf_multi_plot, \
    _latex_header, \
    _latex_footer, \
    do_pgf_stacked_plot, \
    run_pdflatex


from datetime import *
from tools import statistics, statistics_cropped

from pygraph.classes.graph import graph
from pygraph.classes.digraph import digraph

from pygraph.readwrite.dot import write
from pygraph.algorithms.minmax import shortest_path

SHM_REGIONS_OFFSET=20
SHM_SLAVE_START=50

def output_graph(graph, name, algorithm='neato'):
    """
    Output the graph as png image and also as text file
    @param name Name of the file to write to

    """
    dot = write(graph, True)
    gvv = gv.readstring(dot)

    name = 'graphs/%s' % name

    with open('%s.dot'%name, 'w') as f:
        f.write(dot)

    gv.layout(gvv, algorithm)
    gv.render(gvv, 'png', ('%s.png' % name))


def output_quorum_configuration(model, hierarchies, root, sched, topo):
    """
    Output a C array representing overlay and scheduling
    @param hierarchies: List of HybridModules, each of which is responsible for
          sending messages for a group/cluster of cores

    """
    import shm

    d = core_index_dict(model.graph.nodes())
    print d

    dim = model.get_num_cores()
    mat = [[0 for x in xrange(dim)] for x in xrange(dim)]
    
    import hybrid_model

    stream = open("model.h", "w")

    topo = None

    # Build the matrix
    for h in hierarchies:
        assert isinstance(h, hybrid_model.HybridModule)
        if isinstance(h, hybrid_model.MPTree):
            assert sched is not None
            walk_graph(h.graph, root, fill_matrix, mat, sched, d)
            topo = h.graph
        elif isinstance(h, shm.ShmSpmc):
            send_shm(h, mat, d)
        else:
            import pdb; pdb.set_trace()
            raise Error('Unsupported Hybrid Module')

    # Generate c code
    defstream = open("model_defs.h", "w")
    __c_header_model_defs(defstream,
                          d[model.evaluation.last_node],
                          type(model),
                          type(topo),
                          len(mat))
    __c_header_model(stream)
    __matrix_to_c(stream, mat)
    __next_hop_to_c(stream, topo, model);
    __c_footer(stream)
    __c_footer(defstream)


def _reachable_nodes(topo, node, prev):
    c = []
    for n in topo.neighbors(node):
        if n != prev:
            c.append(n)
            c += _reachable_nodes(topo, n, node)
    return c

def __next_hop_to_c(stream, topo, model):
    d = dict()

    if isinstance(topo, digraph):
        topo = graph_from_digraph(topo)

    for c in topo.nodes():
        for n in topo.neighbors(c):
            d[(c,n)] = _reachable_nodes(topo, n, c) + [n]

    dim = model.get_num_cores()
    mat = [[-1 for x in xrange(dim)] for x in xrange(dim)]
    
    for ((sender, next_hop), reachable) in d.items():
        for r in reachable:
            mat[sender][r] = next_hop

    __matrix_to_c(stream, mat, 'next_hop');

    print 'Next-hop table is:'
    print d
                
        

SEND_SHM_IDX=SHM_SLAVE_START
def send_shm(module, mat, core_dict):
    import shm

    global SEND_SHM_IDX
    assert isinstance(module, shm.ShmSpmc)
    assert module.sender in module.receivers
    d = { (recv, module.sender): SEND_SHM_IDX for recv in module.receivers}
    d[(module.sender, module.sender)] += SHM_REGIONS_OFFSET
    # Sender
    for r in module.receivers:
        mat[module.sender][r] = SEND_SHM_IDX + SHM_REGIONS_OFFSET
        if r != module.sender:
            mat[r][module.sender] = SEND_SHM_IDX
    SEND_SHM_IDX += 1
    assert SEND_SHM_IDX<(SHM_SLAVE_START+SHM_REGIONS_OFFSET)


def walk_graph(g, root, func, mat, sched, core_dict):
    """
    Function to walk the tree starting from root

    active = reachable, but not yet dealt with. Elements are tuples (node, parent)
    done = reachable and also handled

    """
    assert isinstance(g, graph) or isinstance(g, digraph)

    active = Queue.Queue()
    done = []

    active.put((root, None))

    while not active.empty():
        # get next
        (a, parent) = active.get()
        assert (not a in done)
        # remember that it was handled
        done.append(a)
        # mark the inactive neighbors as active
        nbs = []
        for nb in g.neighbors(a):
            if not nb in done:
                active.put((nb, a))
                nbs.append(nb)

        # call handler function
        func(a, nbs, parent, mat, sched, core_dict)


def fill_matrix(s, children, parent, mat, sched, core_dict, cost_dict=None):
    """
    @param s: Sending core
    @param children: Children of sending core
    @param parent: Parent node of sending core
    @param mat: Matrix to write at
    @param sched: Scheduler to use or None (in which case messages will be
          send to all nodes in children list in given order). If None,
          weights will be read from cost_dict
    @param core_dict: Dictionary for core name mapping
    @param cost_dict: Dictionary for the integer values to write into matrix
          rather than an integer reflecting the order given by Scheduler. Key of
          the dictionary is (sender, receiver).

    """
    logging.info("%d -> %s"
                 % (core_dict[s], ','.join([ str(c) for c in children ])))
    i = 1

    # Build list of nodes to send the message to
    target_nodes = sched.get_final_schedule(s)

    # Send message
    for (cost, r) in target_nodes:
        logging.info("%d -> %d [%r]" %
                     (core_dict[s], core_dict[r], r in children))
        if r in children:
            mat[core_dict[s]][core_dict[r]] = i
            i += 1
    if not parent == None:
        assert len(children)<90
        mat[core_dict[s]][core_dict[parent]] = 99


def __matrix_to_c(stream, mat, varname='model'):
    """
    Print given matrix as C
    """
    dim = len(mat)
    stream.write("int %s[MODEL_NUM_CORES][MODEL_NUM_CORES] = {\n" % varname)
    # x-axis labens
    stream.write(("//   %s\n" % ' '.join([ "%2d" % x for x in range(dim) ])))
    for x in range(dim):
        stream.write("    {")
        for y in range(dim):
            stream.write("%2d" % mat[x][y])
            if y+1 != dim:
                stream.write(",")
        stream.write("}")
        stream.write(',' if x+1 != dim else ' ')
        stream.write((" // %2d" % x))
        stream.write("\n")
    stream.write("};\n\n")


def __c_header(stream, name):
    stream.write('#ifndef %s\n' % name)
    stream.write('#define %s 1\n\n' % name)

def __c_header_model_defs(stream, last_node, machine, topology, dim):
    __c_header(stream, 'MULTICORE_MODEL_DEFS')
    stream.write('#define MACHINE "%s"\n' % machine)
    stream.write('#define TOPOLOGY "%s"\n' % topology)
    stream.write('#define LAST_NODE %d\n' % last_node)
    stream.write("#define MODEL_NUM_CORES %d\n\n" % dim)

    stream.write('#define SHM_SLAVE_START %d\n' % SHM_SLAVE_START)
    stream.write('#define SHM_SLAVE_MAX %d\n' % 
                 (SHM_SLAVE_START + SHM_REGIONS_OFFSET - 1))
    stream.write('#define SHM_MASTER_START %d\n' % 
                 (SHM_SLAVE_START + SHM_REGIONS_OFFSET));
    stream.write('#define SHM_MASTER_MAX %d\n' %
                 (SHM_SLAVE_START + SHM_REGIONS_OFFSET + SHM_REGIONS_OFFSET - 1));

def __c_header_model(stream):
    __c_header(stream, 'MULTICORE_MODEL')
    stream.write('#include "model_defs.h"\n\n')

def __c_footer(stream):
    stream.write("#endif\n")


def clear_line(line):
    """
    Remove unneeded characters from given string
    """
    return line.rstrip('\r\n').replace('\t', '    ')

def _unpack_line_header(header):
    """
    Format: sk_m_print(<coreid>,<topology>)
    """
    start = "sk_m_print("
    assert header.startswith(start)
    assert header.endswith(")")
    result = str.split((header[len(start): len(header)-1]), ',')
    return (int(result[0]), result[1])


def unpack_line(line):
    """
    Unpacks a measurement line
    Format: sk_m_print(<coreid>,<topology>) idx= <index> tscdiff= <measurement>
    @return tuple (coreid, topology, index, measurement)
    """
    el = str.split(line)
    assert len(el) == 5
    return _unpack_line_header(el[0]) + (int(el[2]), int(el[4]))


def parse_measurement(f, coreids=None):
    """
    Parse the given file for measurements
    """

    print "parse_measurement for file %s" % f
    dic = dict()
    coresfound = []

    # If argument is a path (i.e. string), we need to open it
    if isinstance(f, basestring):
        f = open(f)

    for line in f:
        if line.startswith("sk_m"):
            d = unpack_line(clear_line(line))
            assert len(d)==4
            if coreids == None or d[0] in coreids:
                if not d[0] in dic:
                    dic[d[0]] = []
                dic[d[0]].append(d[3])
                if not d[0] in coresfound:
                    coresfound.append(d[0])
    result = []
    stat = []
    for c in coresfound:
        l = len(dic[c])
        if l > 0:
            print "core %d, length %d" % (c, len(dic[c]))
            s = statistics_cropped(dic[c])
            if s != None:
                stat.append((c, s[0], s[1]))
    return stat


def parse_and_plot_measurement(coreids, machine, topo, f):
    stat = parse_measurement(f, coreids)
    do_pgf_plot(open("../measurements/%s_%s.tex" % (machine, topo), "w+"), stat,
                "Atomic broadcast on %s with %s topology" % (machine, topo),
                "coreid", "cost [cycles]")


def _output_table_header(f):
    f.write(("\\begin{table}[htb]\n"
             "  \\centering\n"
             "  \\begin{tabular}{lrrrrr}\n"
             "  \\toprule\n"
             "  & \\multicolumn{3}{c}{Real hardware [cycles]} & \\multicolumn{2}{c}{Simulation [units]} \\\\\n"
             "  topology & time & factor & stderr & time & factor \\\\\n"
             "  \\midrule\n"
             ))


def _output_table_footer(f, label, caption):
    f.write(("  \\midrule\n"
             "  \\end{tabular}\n"
             "  \\caption{%s}\n"
             "  \\label{tab:%s}\n"
             "\\end{table}\n") % (caption, label))


def _output_table_row(f, item, min_evaluation, min_simulation):
    assert len(item)==4
    fac1 = -1 if float(min_evaluation) == 0 else item[1]/float(min_evaluation)
    fac2 = -1 if float(min_simulation) == 0 else item[3]/float(min_simulation)
    t_sim = item[3]

    if t_sim == sys.maxint:
        t_sim = -1
        fac2 = -1

    f1 = "\colorbox{gray}{%.3f}" % fac1 if fac1 == 1.0 else "%.3f" % fac1
    f2 = "\colorbox{gray}{%.3f}" % fac2 if fac2 == 1.0 else "%.3f" % fac2

    f.write("  %s & %.2f & %s & %.2f & %.0f & %s \\\\\n" %
            (item[0], item[1], f1, item[2], t_sim, f2))

def _wiki_output_table_header(f):
    f.write(("|| ||<-3 :> '''Real hardware''' ||<-3 :> '''Simulation''' ||\n"
             "|| '''topology''' || '''time [cycles]''' || '''factor''' || '''stderr''' || '''time [units]''' || '''factor''' ||\n"
             ))


def _wiki_output_table_footer(f, label, caption):
    f.write("||<-7 : style=\"border:none;\"> Figure: %s||\n" % caption)


def _wiki_output_table_row(f, item, min_evaluation, min_simulation):
    assert len(item)==4
    fac1 = -1 if float(min_evaluation) == 0 else item[1]/float(min_evaluation)
    fac2 = -1 if float(min_simulation) == 0 else item[3]/float(min_simulation)
    t_sim = item[3]

    if t_sim == sys.maxint:
        t_sim = -1
        fac2 = -1

    f1 = "<#99CCFF )>%.3f" % fac1 if fac1 == 1.0 else "<)>%.3f" % fac1
    f2 = "<#99CCFF )>%.3f" % fac2 if fac2 == 1.0 else "<)>%.3f" % fac2

    f.write("|| '''%s''' ||<)> %.2f ||%s ||<)> %.2f ||<)> %.0f ||%s ||\n" %
            (item[0], item[1], f1, item[2], t_sim, f2))


def output_machine_results(machine, res_measurement, res_simulator, flounder=False, umpq=False):
    """
    Generates a LaTeX table for the given result list.

    @param machine Name of the machine
    @param results List of (topology, mean, stderr)
    """

    if len(res_measurement)<1 or len(res_simulator)<1:
        return

    suffix = get_machine_result_suffix(flounder, umpq)
    fname = '../measurements/%s_topologies%s' % (machine, suffix)

    f = open(fname + '.tex', 'w+')
    fwiki = open(fname + '_wiki.txt', 'w+')

    cap = "Topology evaluation for \\%s" % machine

    _output_table_header(f)
    _wiki_output_table_header(fwiki)

    ev_times = [time for (topo, time, err) in res_measurement if time != 0]
    assert len(ev_times)>0
    min_evaluation = min(ev_times)
    min_simulation = min(time for (topo, time) in res_simulator)

    # Otherwise, the simulation didn't work
    assert min_evaluation>0
    assert min_simulation>0

    aliases = dict()
    aliases['adaptivetree'] = 'adaptive'

    for e in zip(res_measurement, res_simulator):

        label = aliases.get(e[0][0], e[0][0])

        assert(e[0][0] == e[1][0])
        _output_table_row(f, (e[0][0], e[0][1], e[0][2], e[1][1]),
                          min_evaluation, min_simulation)
        _wiki_output_table_row(fwiki, (e[0][0], e[0][1], e[0][2], e[1][1]),
                          min_evaluation, min_simulation)

    _output_table_footer(f, machine, cap)
    _wiki_output_table_footer(fwiki, machine, cap)

    f.close()
    fwiki.close()

def extract_machine_results(model, nosim=False, flounder=False, umpq=False):
    """
    Extract result for simulation and real-hardware from log files

    """
    results = []
    sim_results = []
    machine = model.get_name()
    for t in topologies:
        f = get_ab_machine_results(machine, t, flounder, umpq)

        # Real hardware
        if os.path.isfile(f):
            stat = parse_measurement(f, range(model.get_num_cores()))
            assert len(stat) == 1 # Only measurements for one core
            results.append((t, stat[0][1], stat[0][2]))
        else:
            results.append((t, 0, 0))

        # Simulation
        if not nosim:
            try:
                import simulation
                (topo, ev, root, sched, topo) = \
                    simulation._simulation_wrapper(t, model, model.get_graph())
                final_graph = topo.get_broadcast_tree()
                sim_results.append((t, ev.time))
            except:
                print traceback.format_exc()
                print 'Simulation failed for machine [%s] and topology [%s]' %\
                    (machine, t)
                sim_results.append((t, sys.maxint))
        else:
            sim_results.append((t, sys.maxint))

    return (results, sim_results)

def gen_gottardo(m):

    graph = digraph()
    graph.add_nodes([n for n in range(m.get_num_cores())])

    for n in range(1, m.get_num_cores()):
        if n % m.get_cores_per_node() == 0:
            print "Edge %d -> %d" % (0, n)
            graph.add_edge((0, n))

    dim = m.get_num_cores()
    mat = [[0 for x in xrange(dim)] for x in xrange(dim)]

    import sort_longest
    sched = sort_longest.SortLongest(graph)

    # Build the matrix
    walk_graph(graph, 0, fill_matrix, mat, sched)

    stream = open("hybrid_model.h", "w")
    defstream = open("hybrid_model_defs.h", "w")
    __c_header_model_defs(defstream,
                          m.get_num_cores()-1,
                          "",
                          "",
                          len(mat))
    __c_header_model(stream)
    __matrix_to_c(stream, mat)
    __c_footer(stream)
    __c_footer(defstream)


# http://stackoverflow.com/questions/4836710/does-python-have-a-built-in-function-for-string-natural-sort
def natural_sort(l):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
    return sorted(l, key = alphanum_key)


# http://stackoverflow.com/questions/12217537/can-i-force-debugging-python-on-assertionerror
def info(type, value, tb):
   if hasattr(sys, 'ps1') or not sys.stderr.isatty() or type != AssertionError:
      # we are in interactive mode or we don't have a tty-like
      # device, so we call the default hook
      sys.__excepthook__(type, value, tb)
   else:
      import traceback, pdb
      # we are NOT in interactive mode, print the exception...
      traceback.print_exception(type, value, tb)
      print
      # ...then start the debugger in post-mortem mode.
      pdb.pm()

def core_index_dict(n):
    """
    Return a dictionary with indices for cores

    """

    if isinstance(n[0], int):
        return {i: i for i in n}

    n = natural_sort(n)
    return { node: idx for (idx, node) in zip(range(len(n)), n)}


def plot_machine_results(machine, res_measurement, res_sim):
    """
    Generate PGF plot code for the given data
    @param f File to write the code to
    @param data Data points to print as list of tuples (x, y, err)
    """

    fname = '/tmp/bar.tex'
    f = open(fname, "w+")

    _latex_header(f, args=['\\usepgfplotslibrary{external}',
                           '\\tikzexternalize'])

    now = datetime.today()

    xlabels = ','.join([ l for (l, v) in res_sim ])
    args = [ 'ybar', 'ymin=0', 'symbolic x coords={%s}' % xlabels, 'xtick=data',
             'error bars/y dir=both', 'error bars/y explicit',
             'legend style={ at={(0.5,1.03)}, anchor=south }', 
             'legend columns=2' ]

    plotname = "%02d%02d%02d" % (now.year, now.month, now.day)
    _pgf_plot_header(f, plotname, 'topology results for %s' % machine, 
                     'topology', 'cost [cycles]', args)

    minhw = min([x for (_, x, _) in res_measurement])
    minsim = min([x for (_, x) in res_sim])

    sim = [(t, v/minsim*minhw) for (t,v) in res_sim]

    f.write(("    \\addplot coordinates {\n"))
    for (t,v,e) in res_measurement:
        f.write("(%s,%f) +- (%f,%f)\n" % (t,v,e,e))
    f.write(("    };\n"))

    f.write(("    \\addplot coordinates {\n"))
    for (t,v) in sim:
        f.write("(%s,%f)\n" % (t,v))
    f.write(("    };\n"))

    f.write(' \\legend{real hardware, simulation};\n');

    _pgf_plot_footer(f)
    _latex_footer(f)

    f.close() # should flush ..

    run_pdflatex(fname)


def graph_from_digraph(dg):
    
    assert isinstance(dg, digraph)

    g = graph()
    
    for c in dg.nodes():
        g.add_node(c)

    for (s,d) in dg.edges():
        if not (s,d) in g.edges():
            g.add_edge((s,d), dg.edge_weight((s,d)))

    return g
                       
def output_final_graph(overlay):
    """
    Output a pretty dot file with nodes colors indicating NUMA nodes.

    """
    colors = ['aquamarine', 'aquamarine4', 'darkslategray', 'darkslategray4',
              'deepskyblue1', 'deepskyblue3', 'dodgerblue4', 'forestgreen']

    import hybrid_model

    hierarchies = overlay.get_tree()
    for h in hierarchies:
        if isinstance(h, hybrid_model.MPTree):

            g = h.graph
            for n in g.nodes():
                g.add_node_attribute(n, ('style', 'filled'))
                g.add_node_attribute(
                    n, ('color', colors[overlay.mod.get_numa_id(n) % len(colors)]))

            fname = '%s_%s' % (overlay.mod.get_name(), overlay.get_name())
            output_graph(g, fname, 'dot')

            
    
