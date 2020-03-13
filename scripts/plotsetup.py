from matplotlib import rc
import matplotlib

matplotlib.rcParams['text.latex.preamble']=[""]
params = {#'text.usetex' : False,
          'font.size' : 15,
          'font.family' : 'sans-serif',
          #'text.latex.unicode': True,
          }
matplotlib.rcParams.update(params)

matplotlib.rc('xtick', labelsize=15)
matplotlib.rc('ytick', labelsize=15)

matplotlib.rcParams['legend.numpoints'] = 5
#matplotlib.rcParams['legend.scatterpoints'] = 5

# magic legend handler thing that allows us to show a line with background in
# legends
from matplotlib.legend_handler import HandlerLine2D
class HandlerLine2DBG(HandlerLine2D):
    """
    Handler for Line2D instances with colored background
    """
    def __init__(self, marker_pad=0.3, numpoints=None, bgcolor=None, bgalpha=1.0, **kw):
        HandlerLine2D.__init__(self, marker_pad=marker_pad, numpoints=numpoints, **kw)
        self.bgcolor = bgcolor
        self.bgalpha = bgalpha
    def create_artists(self, legend, orig_handle, xdescent, ydescent, width,
            height, fontsize, trans):
        from matplotlib.patches import Rectangle
        ls = super(HandlerLine2DBG,self).create_artists(legend, orig_handle, xdescent,
                ydescent, width, height, fontsize, trans)
        # add bg box behind line
        ls.append(Rectangle((0,0), width, height,
                facecolor=self.bgcolor, alpha=self.bgalpha, linewidth=0))

        return ls
