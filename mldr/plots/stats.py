from collections import defaultdict
from glob import glob
from xml.etree import ElementTree

import numpy as np


def fairness_index(data):
    return np.sum(data) ** 2 / (len(data) * np.sum(data ** 2))


def read_flowstats(file):
    tree = ElementTree.parse(file)
    flowstats = next(iter(tree.getroot()))
    results = []

    for flow in flowstats:
        for bins in flow:
            if bins.tag != 'delayHistogram':
                continue

            x, y = [], []

            for bin in bins:
                x.append(int(1e3 * float(bin.attrib['start'])))
                y.append(float(bin.attrib['count']))

            x, y = np.array(x), np.array(y)
            idx = np.argsort(x)
            x, y = x[idx], y[idx]

            y = y.cumsum() / y.sum()
            y_90 = x[np.argmax(y > 0.9)]
            results.append(y_90)

    return results


if __name__ == '__main__':
    print('High throughput scenario (n = 12)')
    results = defaultdict(list)

    for file in glob('../../scripts/throughput_*mpdu*.csv'):
        with open(file) as f:
            line = f.readline()

        results[' '.join(file.split('_')[1:4])].append(float(line.split(',')[-1]))

    for file in glob('../../scripts/throughput_MLDR_n12_*.csv'):
        if '_log' in file:
            continue

        with open(file) as f:
            line = f.readline()

        results['MLDR'].append(float(line.split(',')[-1]))

    results = {k: results[k] for k in sorted(results)}

    for key, value in results.items():
        print(key, np.round(np.mean(value), 1), np.round(np.std(value), 1))

    print('\nMassive scenario (n = 250)')
    results = defaultdict(list)

    for file in glob('../../scripts/massive_*mpdu*.txt'):
        with open(file) as f:
            lines = list(map(float, f.readlines()))

        results[' '.join(file.split('_')[1:4])].append(fairness_index(np.array(lines)))

    for file in glob('../../scripts/massive_MLDR_n250_*.txt'):
        with open(file) as f:
            lines = list(map(float, f.readlines()))

        results['MLDR'].append(fairness_index(np.array(lines)))

    results = {k: results[k] for k in sorted(results)}

    for key, value in results.items():
        print(key, np.round(np.mean(value), 2), np.round(np.std(value), 2))

    print('\nLow latency scenario (n = 4)')
    results = defaultdict(list)

    for file in glob('../../scripts/latency_*mpdu*.xml'):
        results[' '.join(file.split('_')[1:4])].extend(read_flowstats(file))

    for file in glob('../../scripts/latency_MLDR_n4_*.xml'):
        results['MLDR'].extend(read_flowstats(file))

    results = {k: results[k] for k in sorted(results)}

    for key, value in results.items():
        print(key, np.round(np.mean(value), 1), np.round(np.std(value), 1))
