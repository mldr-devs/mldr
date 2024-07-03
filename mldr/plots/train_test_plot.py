import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

from mldr.plots.config import get_cmap


if __name__ == '__main__':
    df = pd.read_csv('all_logs.csv')
    agents = df['agent'].unique()
    cmap = get_cmap(n=len(agents))

    time_min, time_max = float('inf'), float('-inf')

    for i, (agent, color) in enumerate(zip(agents, cmap)):
        agent_df = df[df['agent'] == agent]
        warmupEndIdx = agent_df['warmupEnd'].argmax()
        warmupEndTime = agent_df['time'].iloc[warmupEndIdx]

        time = agent_df['time'] - warmupEndTime
        time_min = min(time_min, time.min())
        time_max = max(time_max, time.max())

        if i == 0:
            plt.axvline(x=0, color='gray', linestyle='--', label='Warmup end')

        plt.plot(time, agent_df['throughput'], label=agent, color=color)

    plt.xticks(range(int(10 * np.floor(time_min / 10)), int(10 * np.ceil(time_max / 10)) + 1, 10))
    xticks = plt.xticks()
    plt.axvline(x=time_max + 2, color='gray')

    for i, (agent, color) in enumerate(zip(agents, cmap)):
        agent_thr = df[df['agent'] == agent]
        agent_thr = agent_thr[agent_thr['warmupEnd']]
        agent_thr = agent_thr['throughput']

        plt.boxplot(
            agent_thr, positions=[time_max + 3.5 + 1.5 * i], widths=1., patch_artist=True, showfliers=False, showmeans=True,
            boxprops=dict(facecolor=color, linewidth=0.5, edgecolor='gray'),
            whiskerprops=dict(color='gray', linewidth=0.5),
            capprops=dict(color='gray', linewidth=0.5),
            medianprops=dict(color='white', linewidth=0.5),
            meanprops=dict(marker='o', markerfacecolor='white', markeredgecolor='gray', markersize=2, markeredgewidth=0.5)
        )

    plt.xlabel('Time [s]')
    plt.ylabel('Throughput [Mb/s]')
    plt.xlim(time_min, time_max + (time_max - time_min) * 0.17)
    plt.ylim(0, 120)
    plt.yticks(range(0, 121, 30))
    plt.xticks(*xticks)

    plt.legend(ncol=2)
    plt.grid()
    plt.tight_layout()
    plt.savefig('train_test_plot.pdf', bbox_inches='tight')
    plt.show()
