import os
import helpers

machines = 'appenzeller babybel2 gottardo-ht gruyere phi sbrinz1 sgs-r815-03 sgs-r820-01 tomme1 ziger2 pluton'.split()

def export_csv_bars(plots, bars, data, error, fname):
    """Export data as CSV to be used for bar plots on the machine DB website.

    """
    # CSV output
    print(('Writing CSV data to %s' % fname))

    with open(fname, 'w') as fc:

        # Header
        fc.write('algorithm')
        for b in bars:
            fc.write(',%s,%s' %(b, b + '_stderr'))
        fc.write('\n')

        # Data
        for i, p in enumerate(plots):

            fc.write(p)
            for d, e in zip(data[i], error[i]):
                fc.write(',%f,%f' % (d, e))
            fc.write('\n')

        fc.close()




def translate_topo_name(topo):
    """Translate the name of a topology as given by
    the benchmarks into a meaningful printable variant.

    """
    if 'adaptivetree-shuffle-sort' in topo:
        return 'adaptivetree-optimized'

    else:
        return topo

def is_old_revision(m, metadata):

    rev = metadata[m]['smelt-rev'].replace('-dirty', '')

    old = [
        '0480',
        '81ce',
        '1e82',
        'aa33',
        '8c06',
        '92bb',
        'ea19',
        ]

    return True if rev in old else False



def get_adaptive_tree_variant(m, metadata):
    """Return which version of the adaptive tree should be used
    """

    md = metadata[m]

    if m == 'phi':
        # Special treatment - shuffle-sort broken on KNC
        return 'adaptivetree'

    if is_old_revision(m, metadata):
        return 'adaptivetree-nomm-shuffle-sort'

    else:
        # 9a71 is new (tomme1)
        # 3fce is OLD (Xeon Phi)
        # 1e82 is ??? (gottardo-ht)
        return 'adaptivetree-shuffle-sort'


def is_topo_ignore(topo, m, metadata):

    is_old = is_old_revision(m, metadata)
    filter_out = False

    if is_old:
        # For old measurements, we need to:

        # 1) all adaptivetree variants that DON'T use nomm
        if 'adaptive' in topo and not 'nomm' in topo:
            filter_out = True

        # 2) all static topologies EXCEPT naive
        if not 'adaptive' in topo and not 'naive' in topo:
            filter_out = True


    if m == 'phi': # adaptivetree-shuffle-sort does not work on phi
        helpers.warn(('Found phi'))
        filter_out = 'naive' in topo

    # On newer measurements, we don't filter out anything
    if filter_out:
        helpers.info(('Filtering out [%d] topo [%s] on machine [%s]' % (is_old, topo, m)))
    else:
        helpers.note(('Leaving in    [%d] topo [%s] on machine [%s]' % (is_old, topo, m)))


    return filter_out


def configure_matplotlib(presentation=False):

    import matplotlib
    import matplotlib.pyplot as plt

    fontsize = 12

    matplotlib.rcParams['figure.figsize'] = 6.0, 3.5
    plt.rc('legend',**{'fontsize':fontsize, 'frameon': 'false'})
    matplotlib.rcParams.update({'font.size': fontsize,
                            'xtick.labelsize': fontsize,
                            'ytick.labelsize': fontsize})

    if not presentation:
        matplotlib.rc('font', family='serif')
        matplotlib.rc('font', serif='Times New Roman')
        matplotlib.rc('text', usetex='true')

    return fontsize

def get_font(paper=None, ppt=None, fontsize=12):

    if paper == None:
        paper = True if os.environ.get('PAPER') else False
    if ppt == None:
        ppt = True if os.environ.get('PPT') else False

    import matplotlib.font_manager as font_manager

    if ppt:

        fontsize = 18

        # Powerpoint export - Use Calibri

        path = '/home/skaestle/.fonts/calibri.ttf'
        prop = font_manager.FontProperties(fname=path,
                                           weight='100',
                                           size=fontsize)
    else:

        # Else: use LaTeX for rendering
        import matplotlib
        prop = font_manager.FontProperties(family='Times New Roman',
                                           style='normal',
                                           size=fontsize,
                                           weight='normal',
                                           stretch='normal')

        matplotlib.rc('font', family='serif')
        matplotlib.rc('font', serif='Times New Roman')
        matplotlib.rc('text', usetex='true')
        matplotlib.rcParams.update({'font.size': fontsize,
                                    'xtick.labelsize': fontsize,
                                    'ytick.labelsize': fontsize})
        matplotlib.rcParams['font.family'] = prop.get_name()


    prop_small = prop.copy()
    prop_small.set_size(prop.get_size()*0.8)

    return prop, prop_small
