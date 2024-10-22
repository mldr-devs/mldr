import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.stats import t

from mldr.plots.config import get_cmap, AGENT_NAMES


def confidence_interval(data, ci=0.95):
    measurements = len(data)
    mean = data.mean()
    std = data.std()

    alpha = 1 - ci
    z = t.ppf(1 - alpha / 2, measurements - 1)

    ci_low = mean - z * std / np.sqrt(measurements)
    ci_high = mean + z * std / np.sqrt(measurements)

    return mean, ci_low, ci_high


if __name__ == '__main__':
    df = pd.read_csv('../../scripts/thr_results.csv')

    agents = df['agent'].unique()
    cmap = get_cmap(n=len(agents))
    n_wifis = sorted(df['nWifi'].unique())

    for i, agent in enumerate(sorted(agents)):
        agent_mean, agent_low, agent_high = [], [], []

        for n in df['nWifi'].unique():
            results = df[(df['agent'] == agent) & (df['nWifi'] == n)]
            mean, low, high = confidence_interval(results['throughput'])
            agent_mean.append(mean)
            agent_low.append(low)
            agent_high.append(high)

        plt.plot(n_wifis, agent_mean, label=AGENT_NAMES[agent], color=cmap[i], marker='o')
        plt.fill_between(n_wifis, agent_low, agent_high, color=cmap[i], alpha=0.2)

    plt.xlabel('Number of STAs')
    plt.ylabel('Throughput [Mb/s]')
    plt.xlim(0, 21)
    plt.ylim(0, 125)
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'thr_plot.pdf', bbox_inches='tight')
    plt.show()
