import os

import matplotlib.pyplot as plt
import pandas as pd

L1_size = 49152
L2_size = 524288
L3_size = 12582912

df = pd.read_csv("./res.csv", sep=";", index_col=0)
fig, ax = plt.subplots()
df.plot(ax=ax)


def annotate(pos, txt):
    ax.text(
        pos,
        0.99,
        txt,
        rotation=90,
        transform=ax.get_xaxis_transform(),
        ha="right",
        va="top",
    )


ax.axvline(x=L1_size, linestyle="--")
annotate(L1_size, "L1")

ax.axvline(x=L2_size, linestyle="--")
annotate(L2_size, "L2")

ax.axvline(x=L3_size, linestyle="--")
annotate(L3_size, "L3")

ax.legend()
ax.set_xscale("log", base=2)
ax.set_ylabel("time (ns)")
plt.show()
