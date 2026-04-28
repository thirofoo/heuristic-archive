import matplotlib.pyplot as plt
import numpy as np


def gauss(x, a, mu, sigma):
    return a * np.exp(-(x - mu)**2 / (2 * sigma**2))

fig = plt.figure()

A = fig.add_subplot(111)
A.grid(color="k", linestyle="dotted")
A.set_title("gauss function", fontsize=16)
A.set_xlabel("x", fontsize=14)
A.set_ylabel("y", fontsize=14)

mu = 25
sigma = mu / 3
xlim = mu * 2
ylim = 2

A.set_xlim([mu - xlim, mu + xlim])
A.set_ylim([0, ylim])

x = np.arange(-xlim, xlim, 0.1)

y1 = gauss(x, 1, mu, sigma)

A.plot(x, y1, color="deeppink", label="mu=%d, sigma=%d" % (mu, sigma))

A.legend()

# Save the figure instead of showing it
plt.savefig('gauss_plot.png')
