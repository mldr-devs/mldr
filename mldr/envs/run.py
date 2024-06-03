import os
os.environ['JAX_ENABLE_X64'] = 'True'

from argparse import ArgumentParser

import numpy as np
from py_interface import *
from reinforced_lib import RLib
from reinforced_lib.agents.mab import *
from reinforced_lib.exts import BasicMab

from mldr.envs.ns3_ai_structures import Env, Act


PNAME = 'scenario'
MEMBLOCK_KEY = 2333
MEM_SIZE = 64

N_CW = 7
N_RTS_CTS = 2
N_AMPDU = 2

AGENT_ARGS = {
    'EGreedy': {
        'e': 0.05,
        'optimistic_start': 100.0
    },
    'UCB': {
        'c': 1.0
    },
    'NormalThompsonSampling': {
        'alpha': 1.0,
        'beta': 1.0,
        'mu': 100.0,
        'lam': 0.0,
    }
}


if __name__ == '__main__':
    args = ArgumentParser()

    # global settings
    args.add_argument('--seed', type=int, default=100)
    args.add_argument('--mempoolKey', type=int, default=1234)
    args.add_argument('--ns3Path', type=str, default='')

    # ns-3 args
    args.add_argument('--agentName', type=str, required=True)
    args.add_argument('--channelWidth', type=int, default=20)
    args.add_argument('--csvPath', type=str, default='results.csv')
    args.add_argument('--dataRate', type=int, default=150)
    args.add_argument('--distance', type=float, default=10.0)
    args.add_argument('--fuzzTime', type=float, default=5.0)
    args.add_argument('--interactionTime', type=float, default=0.5)
    args.add_argument('--nWifi', type=int, default=1)
    args.add_argument('--packetSize', type=int, default=1500)
    args.add_argument('--simulationTime', type=float, default=50.0)
    args.add_argument('--warmupTime', type=float, default=10.0)

    # reward weights
    args.add_argument('--massive', type=float, default=0.0)
    args.add_argument('--throughput', type=float, default=1.0)
    args.add_argument('--urllc', type=float, default=0.0)

    args = args.parse_args()
    args = vars(args)

    # read the arguments
    ns3_path = args.pop('ns3Path')

    if os.environ.get('NS3_DIR'):
        ns3_path = os.environ['NS3_DIR']
    if not ns3_path:
        raise ValueError('ns-3 path not found')

    agent = args['agentName']
    seed = args.pop('seed')
    mempool_key = args.pop('mempoolKey')
    ns3_args = args

    massive_weight = args.pop('massive')
    throughput_weight = args.pop('throughput')
    urllc_weight = args.pop('urllc')

    # set up the agent
    if agent not in AGENT_ARGS:
        raise ValueError('Invalid agent type')

    rlib = RLib(
        agent_type=globals()[agent],
        agent_params=AGENT_ARGS[agent],
        ext_type=BasicMab,
        ext_params={'n_arms': N_CW * N_RTS_CTS * N_AMPDU}
    )
    rlib.init(seed)

    # set up the environment
    exp = Experiment(mempool_key, MEM_SIZE, PNAME, ns3_path, using_waf=False)
    var = Ns3AIRL(MEMBLOCK_KEY, Env, Act)

    try:
        # run the experiment
        ns3_process = exp.run(setting=ns3_args, show_output=True)

        while not var.isFinish():
            with var as data:
                if data is None:
                    break

                action = rlib.sample(
                    massive_weight * data.env.fairness +
                    throughput_weight * data.env.throughput +
                    urllc_weight * (data.env.latency + data.env.plr) / 2
                )

                cw, rts_cts, ampdu = np.unravel_index(action, (N_CW, N_RTS_CTS, N_AMPDU))
                data.act.cw = cw
                data.act.rts_cts = rts_cts
                data.act.ampdu = ampdu

        ns3_process.wait()
    finally:
        del exp
