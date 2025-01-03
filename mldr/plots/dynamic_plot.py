from glob import glob

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

from mldr.plots.config import get_cmap, AGENT_NAMES, COLUMN_WIDTH, COLUMN_HIGHT


if __name__ == '__main__':
    min_wifi = 5
    n_wifi = 50
    interaction_time = 0.5
    interval_sta = 5
    interval_time = 100 / interaction_time
    n_steps = (n_wifi - min_wifi) // interval_sta + 1

    cmap = get_cmap(n=len(AGENT_NAMES))
    agent_results = {}

    for file in glob(f'../../scripts/dynamic_*_m*_n*_s*.csv'):
        df = pd.read_csv(file)
        agent = file.split('_')[1:3]
        agent = '_'.join(agent) if agent[1] == 'RTS' else agent[0]
        agent = AGENT_NAMES[agent]
        agent_results[agent] = df.iloc[3:, -2].values.reshape(-1, 4).mean(axis=1)

    _, ax = plt.subplots(figsize=(2 * COLUMN_WIDTH, COLUMN_HIGHT))

    for i in range(1, n_steps):
        ax.axvline(i * interval_time, color='red', linewidth=0.5, linestyle='--')

    for i, (agent, results) in enumerate(sorted(agent_results.items())):
        xs = np.linspace(0, interval_time * n_steps, len(results))
        ax.plot(xs, results, color=cmap[i], label=agent)

    ax2 = ax.twinx()
    x = sum([2 * [i * interval_time] for i in range(n_steps + 1)], [])[1:-1]
    y = sum([2 * [min_wifi + i * interval_sta] for i in range(n_steps)], [])
    ax2.plot(x, y, color='black', linewidth=0.5)
    ax2.set_ylim(0, n_wifi)
    ax2.set_ylabel('Number of stations')

    ax.set_xlabel('Steps')
    ax.set_ylabel('Aggregated throughput [Mb/s]')
    ax.set_ylim(0, 125)
    ax.set_xlim(0, xs[-1])
    ax.set_xticks(range(0, int(xs[-1]) + 1, 200))
    ax.grid()

    ax.plot([None], [None], color='black', linewidth=0.5, label='Number of stations')
    ax.plot([None], [None], color='r', linewidth=0.5, linestyle='--', label='Agent reset')
    ax.legend(ncol=5, bbox_to_anchor=(0.55, 0.1), loc='center')

    plt.tight_layout()
    plt.savefig(f'dynamic.pdf', bbox_inches='tight')
    plt.show()
