from collections import defaultdict
from glob import glob

import matplotlib.pyplot as plt
import numpy as np

from mldr.plots.config import get_cmap, AGENT_NAMES


if __name__ == '__main__':
    cmap = get_cmap(n=len(AGENT_NAMES))
    agent_results = defaultdict(lambda: defaultdict(float))

    for file in glob(f'../../scripts/massive_*_n*_d*.txt'):
        with open(file) as f:
            lines = f.readlines()

        agent = file.split('_')[1:3]
        agent = '_'.join(agent) if agent[1] == 'RTS' else agent[0]
        agent = AGENT_NAMES[agent]

        for val in lines:
            agent_results[agent][1e3 * float(val)] += 1

    for i, (agent, results) in enumerate(sorted(agent_results.items())):
        xs, ys = list(results.keys()), list(results.values())
        xs, ys = np.array(xs), np.array(ys)
        idx = np.argsort(xs)
        xs, ys = xs[idx], ys[idx]
        xs, ys = np.concatenate(([0], xs)), np.concatenate(([0], ys))
        served = ys.cumsum() / ys.sum()
        plt.plot(xs, served, label=agent, color=cmap[i], linewidth=1)

    plt.ylabel('CDF')
    plt.xlabel('Throughput per station [kb/s]')
    plt.ylim(0, 1)
    plt.xlim(0, xs[-1])
    plt.grid()
    plt.legend(loc='upper left')
    plt.tight_layout()
    plt.savefig(f'massive_cdf.pdf', bbox_inches='tight')
    plt.show()
