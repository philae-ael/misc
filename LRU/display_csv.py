import matplotlib.cm as cm
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

df = pd.read_csv("./out.csv", sep=";")


fig = plt.figure()


def slope(x, y):
    return np.polyfit(x, y, 1)[0]


def display(ax, c, df, v):
    ips = df["lru_size"] / df["item_count"]
    missrate = df["missrate"]
    ax.plot(ips, missrate, color=c)
    ax.set_ylabel("missrate", color=c)
    ax.set_xlabel("lru size per item count", color=c)

    cond = missrate > 0.1
    print(v / df["lru_size"].iloc[1], slope(ips[cond], missrate[cond]))


# ax = fig.add_subplot()
# iter_counts = df["iter_count"].unique()
# colors = matplotlib.colormaps["hsv"]
# for i, v in enumerate(iter_counts):
#     display(ax, colors(i / len(iter_counts)), df[df["iter_count"] == v], v)
#


ips = df["lru_size"] / df["item_count"]
missrate = df["missrate"]
ax = fig.add_subplot(111, projection="3d")
# ax.scatter(ips, hitrate, df.iter_count, cmap=cm.jet, c=df.iter_count)
ax.plot_trisurf(ips, missrate, df.iter_count, linewidth=0.2, cmap=cm.jet)
ax.set_xlabel("lru size per item count")
ax.set_ylabel("missrate")
ax.set_zlabel("iter_count")

fig.tight_layout()
plt.show()
