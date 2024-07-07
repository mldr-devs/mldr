from argparse import ArgumentParser
from collections import defaultdict
from glob import glob
from xml.etree import ElementTree

import pandas as pd

from mldr.plots.config import get_cmap


if __name__ == '__main__':
    args = ArgumentParser()
    args.add_argument('--dataRate', type=int, required=True)
    args.add_argument('--nWifi', type=int, required=True)
    args.add_argument('--latencyThr', type=float, default=0.005)
    args.add_argument('--reliabilityThr', type=float, default=0.99)
    args = args.parse_args()

    print(f'Latency threshold: {1000 * args.latencyThr} ms')
    print(f'Reliability threshold: {100 * args.reliabilityThr}%\n')

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
                agent_results[agent].append([])

                for bins in flow:
                    if bins.tag != 'delayHistogram':
                        continue

                    for bin in bins:
                        start, width, count = float(bin.attrib['start']), float(bin.attrib['width']), float(bin.attrib['count'])
                        agent_results[agent][-1].append((start, width, count))

    for agent, results in agent_results.items():
        served_devices = 0
        print(f'Agent: {agent}')

        for i, flow in enumerate(results):
            df = pd.DataFrame(flow, columns=['start', 'width', 'count'])
            end, count = df['start'].values + df['width'].values, df['count'].values
            rel = count[end <= args.latencyThr].sum() / count.sum()

            print(f'STA {i} - reliability: {100 * rel:.2f}%')

            if rel >= args.reliabilityThr:
                served_devices += 1

        print(f'Served devices: {served_devices}/{len(results)}\n')
