from argparse import ArgumentParser
from collections import defaultdict
from glob import glob
from xml.etree import ElementTree

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from mldr.plots.config import get_cmap


if __name__ == '__main__':
    args = ArgumentParser()
    args.add_argument('--dataRate', type=int, required=True)
    args.add_argument('--nWifi', type=int, required=True)
    args = args.parse_args()

    agent_files = defaultdict(list)
    agent_results = defaultdict(list)

    for file in glob(f'flowmon_*_n{args.nWifi}_r{args.dataRate}_s*.xml'):
        agent = file.split('_')[1]
        agent_files[agent].append(file)

    cmap = get_cmap(n=len(agent_files))

    for i, (agent, files) in enumerate(agent_files.items()):
        for file in files:
            tree = ElementTree.parse(file)
            flowstats = next(iter(tree.getroot()))

            for flow in flowstats:
                for bins in flow:
                    if bins.tag != 'delayHistogram':
                        continue

                    for bin in bins:
                        start, width, count = float(bin.attrib['start']), float(bin.attrib['width']), float(bin.attrib['count'])
                        agent_results[agent].append((start, width, count))

    for i, (agent, results) in enumerate(agent_results.items()):
        df = pd.DataFrame(results, columns=['start', 'width', 'count'])
        df = df.groupby('start').sum()
        start, count = df.index.values, df['count'].values
        start, count = np.concatenate(([0], start)), np.concatenate(([0], count))

        count = np.cumsum(count) / count.sum()
        plt.plot(start, count, label=agent, color=cmap[i], linewidth=1)

    plt.xlabel('Latency [s]')
    plt.ylabel('CDF')
    plt.xlim(left=0)
    plt.ylim(0, 1)
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'latency_cdf_plot_n{args.nWifi}_r{args.dataRate}.pdf', bbox_inches='tight')
    plt.show()
