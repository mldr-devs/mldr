import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

from mldr.plots.config import get_cmap, AGENT_NAMES


if __name__ == '__main__':
    df = pd.read_csv('../../scripts/train_test_logs.csv')
    agents = df['agent'].unique()
    cmap = get_cmap(n=len(AGENT_NAMES))[::-1]
    df['latency'] = df['latency'] * 1000

    idx_min, idx_max = float('inf'), float('-inf')

    for agent in agents:
        agent_df = df[df['agent'] == agent]
        idx_min = min(idx_min, len(agent_df))
        idx_max = max(idx_max, len(agent_df))

    plt.axvline(x=idx_max - idx_min, color='gray', linestyle='--', label='Warmup end')
    xs = np.arange(idx_max)

    for i, (agent, color) in enumerate(zip(agents, cmap)):
        agent_df = df[df['agent'] == agent]
        plt.plot(xs[idx_max - len(agent_df):], agent_df['latency'], label=AGENT_NAMES[agent], color=color)

    plt.xticks(range(0, int(5 * np.ceil(idx_max / 5)), 50))
    xticks = plt.xticks()
    plt.axvline(x=idx_max, color='gray')

    for i, (agent, color) in enumerate(zip(agents, cmap)):
        agent_thr = df[df['agent'] == agent]
        agent_thr = agent_thr[agent_thr['warmupEnd'] == 1]
        agent_thr = agent_thr['latency'].values

        plt.boxplot(
            agent_thr, positions=[idx_max + 7 + 8 * i], widths=2., patch_artist=True, showfliers=False, showmeans=True,
            boxprops=dict(facecolor=color, linewidth=0.5, edgecolor='gray'),
            whiskerprops=dict(color='gray', linewidth=0.5),
            capprops=dict(color='gray', linewidth=0.5),
            medianprops=dict(color='white', linewidth=0.5),
            meanprops=dict(marker='o', markerfacecolor='white', markeredgecolor='gray', markersize=2, markeredgewidth=0.5)
        )

    plt.xlabel('Step')
    plt.ylabel('Latency [ms]')
    plt.xlim(0, 370)
    plt.yscale('log')
    plt.xticks(*xticks)

    plt.legend(ncol=2)
    plt.grid()
    plt.tight_layout()
    plt.savefig('latency_train-test.pdf', bbox_inches='tight')
    plt.show()
