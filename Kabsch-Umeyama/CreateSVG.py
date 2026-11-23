# Lets create an svg file using that math card spec
import numpy as np
import math
import svgwrite

output_path = "/Users/kneenaugh/Desktop/Pop-up Card Research/Mathematical-Pop-Up-Card-Simulator/Kabsch-Umeyama/test.svg"
dwg = svgwrite.Drawing( output_path, size=("1000px", "1000px"), viewBox="-32 -32 64 64")

lw = 0.05
d = math.sqrt(2)/2
r = 0.15

# Draw Base Card
baseCard = np.array([
    [0.0, -1.0, 0.0],  # 0
    [-1.0, -1.0, 0.0], # 1
    [-1.0, 1.0, 0.0],  # 2
    [0.0, 1.0, 0.0],   # 3
    [1.0, 1.0, 0.0],   # 4
    [1.0, -1.0, 0.0]   # 5
])

baseCard = [(float(x), float(-y)) for x, y,z in baseCard] # flip y for svg purposes
dwg.add( dwg.polygon(points=baseCard, fill="none", stroke="black", stroke_width=lw) )
dwg.add( dwg.line( start=baseCard[0], end=baseCard[3], stroke="blue", stroke_width=lw) )

# Draw Mech 1
mech1 = np.array([
    [5.0, 0.0, 0.0],  # 0
    [4.0, 0.0, 0.0], # 1
    [4.0, 1.0, 0.0],  # 2
    [5.0, 1.0, 0.0],   # 3
    [6.0, 1.0, 0.0],   # 4
    [6.0, 0.0, 0.0]   # 5
])

mech1 = [(float(x), float(-y)) for x, y,z in mech1] # flip y for svg purposes
dwg.add( dwg.polygon(points=mech1, fill="none", stroke="black", stroke_width=lw) )
dwg.add( dwg.line( start=mech1[0], end=mech1[3], stroke="red", stroke_width=lw) )

# Draw Mech 2
mech2 = np.array([
    [5.0, -2.0, 0.0],  # 0
    [5.0+d, -2.0+d, 0.0], # 1
    [5.0, -2.0+d, 0.0],  # 2
    [5.0-d, -2.0+d, 0.0],   # 3
])

mech2 = [(float(x), float(-y)) for x, y,z in mech2] # flip y for svg purposes
dwg.add( dwg.polygon(points=mech2, fill="none", stroke="black", stroke_width=lw) )
dwg.add( dwg.line( start=mech2[0], end=mech2[2], stroke="red", stroke_width=lw) )

# Draw glue dots
glueDots = np.array([
    [-d, d, 0.0],    # mech 1 vert 1 onto mech 0 
    [0.0, 0.0, 0.0], # ORIGIN mech 1 vert 0 onto mech 0 
    [d, d, 0.0],     # mech 1 vert 5 onto mech 0 
    [5.0+d, 0.0+d, 0.0],   # mech 2 vert 1 onto mech 1
    #[0.0, 0.0, 0.0], # (origin) mech 2 vert 0 onto mech 0
    [0.0+1.0, 0.0, 0.0] # mech 2 vert 3 onto mech 0
])

glueDots = [(float(x), float(-y)) for x, y,z in glueDots] # flip y for svg purposes
dwg.add( dwg.circle( center=glueDots[0], r=r, stroke="rgb(0,255,255)", stroke_width=lw, fill="none" ) ) # mech 1 vert 1 onto mech 0 
dwg.add( dwg.circle( center=mech1[1], r=r, stroke="rgb(0,255,255)", stroke_width=lw, fill="none") ) # mech 1 vert 1

dwg.add( dwg.circle( center=glueDots[1], r=r, stroke="rgb(0,254,255)", stroke_width=lw, fill="none" ) ) # ORIGIN mech 1 vert 0 onto mech 0 
dwg.add( dwg.circle( center=mech1[0], r=r, stroke="rgb(0,254,255)", stroke_width=lw, fill="none") ) # ORIGIN mech 1 vert 0

dwg.add( dwg.circle( center=glueDots[2], r=r, stroke="rgb(0,253,255)", stroke_width=lw, fill="none" ) ) # mech 1 vert 5 onto mech 0 
dwg.add( dwg.circle( center=mech1[5], r=r, stroke="rgb(0,253,255)", stroke_width=lw, fill="none") ) # mech 1 vert 5

dwg.add( dwg.circle( center=glueDots[3], r=r, stroke="rgb(0,252,255)", stroke_width=lw, fill="none" ) ) # mech 2 vert 1 onto mech 1
dwg.add( dwg.circle( center=mech2[1], r=r, stroke="rgb(0,252,255)", stroke_width=lw, fill="none") ) # # mech 2 vert 1

dwg.add( dwg.circle( center=mech2[0], r=r, stroke="rgb(0,254,255)", stroke_width=lw, fill="none") ) # (origin) mech 2 vert 0

dwg.add( dwg.circle( center=glueDots[4], r=r, stroke="rgb(0,251,255)", stroke_width=lw, fill="none" ) ) # mech 2 vert 3 onto mech 0
dwg.add( dwg.circle( center=mech2[3], r=r, stroke="rgb(0,251,255)", stroke_width=lw, fill="none") ) # mech 2 vert 3


# facet lines!!!


dwg.save()