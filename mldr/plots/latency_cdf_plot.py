from collections import defaultdict
from glob import glob
from xml.etree import ElementTree

import matplotlib.pyplot as plt
import numpy as np

from mldr.plots.config import get_cmap


AGENT_NAMES = {
    '80211': '802.11',
    '80211_RTS': '802.11 RTS/CTS',
    'MLDR': 'MLDR'
}


if __name__ == '__main__':
    cmap = get_cmap(n=len(AGENT_NAMES))
    agent_files = defaultdict(list)
    agent_results = defaultdict(dict)

    for file in glob(f'../../scripts/latency_*_n*_r*.xml'):
        agent = file.split('_')[1:3]
        agent = '_'.join(agent) if agent[1] == 'RTS' else agent[0]
        agent = AGENT_NAMES[agent]
        agent_files[agent].append(file)

    for agent, files in agent_files.items():
        for file in files:
            tree = ElementTree.parse(file)
            flowstats = next(iter(tree.getroot()))

            for flow in flowstats:
                for bins in flow:
                    if bins.tag != 'delayHistogram':
                        continue

                    for bin in bins:
                        agent_results[agent][int(1e3 * float(bin.attrib['start']))] = float(bin.attrib['count'])

    for i, (agent, results) in enumerate(sorted(agent_results.items())):
        xs, ys = list(results.keys()), list(results.values())
        xs, ys = np.array(xs), np.array(ys)
        latency = ys.cumsum() / ys.sum()
        plt.plot(xs, latency, label=agent, color=cmap[i], linewidth=1)

    plt.xlabel('Latency [ms]')
    plt.ylabel('CDF')
    plt.xlim(0, 100)
    plt.ylim(0, 1)
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'latency_cdf.pdf', bbox_inches='tight')
    plt.show()
