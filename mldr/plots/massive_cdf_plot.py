from argparse import ArgumentParser
from collections import defaultdict
from glob import glob

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from mldr.plots.config import get_cmap


if __name__ == '__main__':
    args = ArgumentParser()
    args.add_argument('--nWifi', type=int, required=True)
    args = args.parse_args()

    agent_results = defaultdict(list)

    for file in glob(f'*_massive_*{args.nWifi}.txt'):
        with open(file) as f:
            lines = f.readlines()

        agent = file.split('_')[1]
        agent_results[agent] = sorted(list(map(lambda x: (float(x), 1), lines)))

    cmap = get_cmap(n=len(agent_results))

    for i, (agent, results) in enumerate(sorted(agent_results.items())):
        df = pd.DataFrame(results, columns=['start', 'count'])
        df = df.groupby('start').sum()
        start, count = df.index.values, df['count'].values
        start, count = np.concatenate(([0], start)), np.concatenate(([0], count))
        count = np.cumsum(count)
        plt.plot(count, 1000 * start, label=agent, color=cmap[i], linewidth=1)

    plt.xlabel('Devices')
    plt.ylabel('Throughput per device [kb/s]')
    plt.ylim(0, 50)
    plt.xlim(0, 250)
    plt.grid()
    plt.legend(loc='upper left')
    plt.tight_layout()
    plt.savefig(f'massive_cdf_{args.nWifi}.pdf', bbox_inches='tight')
    plt.show()
