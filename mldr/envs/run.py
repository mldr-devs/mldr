import os
os.environ['JAX_ENABLE_X64'] = 'True'

from argparse import ArgumentParser
from collections import deque

import jax
import numpy as np
import tensorflow_probability.substrates.jax as tfp
from py_interface import *
from reinforced_lib import RLib
from reinforced_lib.agents.mab import *
from reinforced_lib.exts import BasicMab
from reinforced_lib.logs import *

from mldr.envs.ns3_ai_structures import Env, Act


PNAME = 'scenario'
MEMBLOCK_KEY = 2333
MEM_SIZE = 128

N_CW = 7
N_RTS_CTS = 2
N_AMPDU = 2

ACTION_HISTORY_LEN = 20
ACTION_PROB_THRESHOLD = 0.9
LATENCY_THRESHOLD = 0.1

AGENT_ARGS = {
    'EGreedy': {
        'e': 0.05,
        'optimistic_start': 1.0
    },
    'UCB': {
        'c': 0.01
    },
    'NormalThompsonSampling': {
        'alpha': 1.0,
        'beta': 1.0,
        'mu': 1.0,
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
    args.add_argument('--agentName', type=str, default='UCB')
    args.add_argument('--channelWidth', type=int, default=20)
    args.add_argument('--csvPath', type=str, default='results.csv')
    args.add_argument('--dataRate', type=int, default=110)
    args.add_argument('--distance', type=float, default=10.0)
    args.add_argument('--fuzzTime', type=float, default=5.0)
    args.add_argument('--interactionTime', type=float, default=0.5)
    args.add_argument('--nWifi', type=int, default=1)
    args.add_argument('--packetSize', type=int, default=1500)
    args.add_argument('--simulationTime', type=float, default=50.0)

    # reward weights
    args.add_argument('--massive', type=float, default=0.0)
    args.add_argument('--throughput', type=float, default=1.0)
    args.add_argument('--urllc', type=float, default=0.0)

    # agent settings
    args.add_argument('--maxWarmup', type=int, default=50.0)
    args.add_argument('--useWarmup', default=False, action='store_true')

    args = args.parse_args()
    args = vars(args)

    # read the arguments
    ns3_path = args.pop('ns3Path')

    if os.environ.get('NS3_DIR'):
        ns3_path = os.environ['NS3_DIR']
    if not ns3_path:
        raise ValueError('ns-3 path not found')

    seed = args.pop('seed')
    key = jax.random.PRNGKey(seed)

    agent = args['agentName']
    mempool_key = args.pop('mempoolKey')

    ns3_args = args
    ns3_args['RngRun'] = seed

    # set up the reward distribution and warmup
    reward_probs = [args.pop('massive'), args.pop('throughput'), args.pop('urllc')]
    reward_dist = tfp.distributions.Categorical(probs=reward_probs)

    def normalize_rewards(env):
        fairness, throughput, latency = env.fairness, env.throughput, env.latency

        fairness = None
        throughput = throughput / args['dataRate']
        latency = max(0, 1 - latency / LATENCY_THRESHOLD)

        return fairness, throughput, latency

    max_warmup = args.pop('maxWarmup')
    use_warmup = args.pop('useWarmup')

    action_history = {
        'cw': deque(maxlen=ACTION_HISTORY_LEN),
        'rts_cts': deque(maxlen=ACTION_HISTORY_LEN),
        'ampdu': deque(maxlen=ACTION_HISTORY_LEN)
    }

    def end_warmup(cw, rts_cts, ampdu, time):
        if not use_warmup or time > max_warmup:
            return True

        action_history['cw'].append(cw)
        action_history['rts_cts'].append(rts_cts)
        action_history['ampdu'].append(ampdu)

        if len(action_history['cw']) < ACTION_HISTORY_LEN:
            return False

        max_prob = lambda actions: (np.unique(actions, return_counts=True)[1] / len(actions)).max()

        if min(max_prob(action_history['cw']), max_prob(action_history['rts_cts']), max_prob(action_history['ampdu'])) > ACTION_PROB_THRESHOLD:
            return True

        return False

    # set up the agent
    if agent == 'wifi':
        rlib = None
    elif agent not in AGENT_ARGS:
        raise ValueError('Invalid agent type')
    else:
        rlib = RLib(
            agent_type=globals()[agent],
            agent_params=AGENT_ARGS[agent],
            ext_type=BasicMab,
            ext_params={'n_arms': N_CW * N_RTS_CTS * N_AMPDU},
            logger_types=CsvLogger,
            logger_params={'csv_path': f'rlib_{args["csvPath"]}'},
            logger_sources=('reward', SourceType.METRIC)
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

                key, subkey = jax.random.split(key)
                reward_id = reward_dist.sample(seed=subkey)
                rewards = normalize_rewards(data.env)

                action = rlib.sample(rewards[reward_id])
                cw, rts_cts, ampdu = np.unravel_index(action, (N_CW, N_RTS_CTS, N_AMPDU))

                rlib.log('cw', cw)
                rlib.log('rts_cts', rts_cts)
                rlib.log('ampdu', ampdu)

                data.act.cw = cw
                data.act.rts_cts = rts_cts
                data.act.ampdu = ampdu
                data.act.end_warmup = end_warmup(cw, rts_cts, ampdu, data.env.time)

        ns3_process.wait()
    finally:
        del exp
        del rlib
