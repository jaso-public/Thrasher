import matplotlib.pyplot as plt
import numpy as np

xpoints = np.random.randint(250, size=100)
ypoints = np.random.randint(250, size=100)

colour = (xpoints/xpoints.max())*50+(ypoints/ypoints.max())*50

plt.ion()
plt.plot([0,100],[100,0])
plt.scatter(xpoints, ypoints, c = colour)
plt.show()


