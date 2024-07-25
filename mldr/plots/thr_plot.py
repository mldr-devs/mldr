import matplotlib.pyplot as plt
import pandas as pd

from mldr.plots.config import get_cmap


if __name__ == '__main__':
    df = pd.read_csv('thr-results.csv')

    agents = df['agent'].unique()
    cmap = get_cmap(n=len(agents))

    for i, agent in enumerate(sorted(agents)):
        results = df[df['agent'] == agent]
        plt.plot(results['nWifi'], results['throughput'], label=agent, color=cmap[i], marker='o')

    plt.xlabel('Number of STAs')
    plt.ylabel('Throughput [Mb/s]')
    plt.xlim(0, 21)
    plt.ylim(0, 125)
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'thr_plot.pdf', bbox_inches='tight')
    plt.show()
