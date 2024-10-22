import os
os.environ['JAX_ENABLE_X64'] = 'True'

import argparse
from collections import deque

import numpy as np
from py_interface import *
from reinforced_lib import RLib
from reinforced_lib.agents.mab import ThompsonSampling
from reinforced_lib.logs import *

from mldr.envs.ns3_ai_structures import Env, Act


MEMBLOCK_KEY = 2333
MEM_SIZE = 128

N_CW = 7
N_RTS_CTS = 2
N_AMPDU = 2

ACTION_HISTORY_LEN = 20
ACTION_PROB_THRESHOLD = 0.9
FAIRNESS_SCALE = 10.0
THROUGHPUT_SCALE = 1.0
LATENCY_SCALE = 3.0
REWARD_SCALE = 3.0


if __name__ == '__main__':
    args = argparse.ArgumentParser()

    # global settings
    args.add_argument('--seed', type=int, default=100)
    args.add_argument('--mempoolKey', type=int, default=1234)
    args.add_argument('--ns3Path', type=str, default='')
    args.add_argument('--scenario', type=str, default='scenario')

    # ns-3 args
    args.add_argument('--agentName', type=str, default='MLDR')
    args.add_argument('--ampdu', action=argparse.BooleanOptionalAction, default=True)
    args.add_argument('--channelWidth', type=int, default=20)
    args.add_argument('--csvLogPath', type=str, default='logs.csv')
    args.add_argument('--csvPath', type=str, default='results.csv')
    args.add_argument('--cw', type=int, default=-1)
    args.add_argument('--dataRate', type=int, default=120)
    args.add_argument('--distance', type=float, default=10.0)
    args.add_argument('--flowmonPath', type=str, default='flowmon.xml')
    args.add_argument('--fuzzTime', type=float, default=5.0)
    args.add_argument('--interactionTime', type=float, default=0.5)
    args.add_argument('--interPacketInterval', type=float, default=0.5)
    args.add_argument('--maxQueueSize', type=int, default=100)
    args.add_argument('--mcs', type=int, default=0)
    args.add_argument('--nWifi', type=int, default=10)
    args.add_argument('--packetSize', type=int, default=1500)
    args.add_argument('--rtsCts', action=argparse.BooleanOptionalAction, default=False)
    args.add_argument('--simulationTime', type=float, default=50.0)
    args.add_argument('--thrPath', type=str, default='thr.txt')

    # reward weights
    args.add_argument('--massive', type=float, default=0.0)
    args.add_argument('--throughput', type=float, default=1.0)
    args.add_argument('--urllc', type=float, default=0.0)

    # agent settings
    args.add_argument('--maxWarmup', type=int, default=100.0)
    args.add_argument('--useWarmup', action=argparse.BooleanOptionalAction, default=False)

    args = args.parse_args()
    args = vars(args)

    # read the arguments
    ns3_path = args.pop('ns3Path')

    if args['scenario'] == 'scenario':
        del args['interPacketInterval']
        del args['mcs']
        del args['thrPath']
        dataRate = min(115, args['dataRate'] * args['nWifi'])
    elif args['scenario'] == 'adhoc':
        del args['dataRate']
        del args['maxQueueSize']
        dataRate = (args['packetSize'] * args['nWifi'] / args['interPacketInterval']) / 1e6

    if os.environ.get('NS3_DIR'):
        ns3_path = os.environ['NS3_DIR']
    if not ns3_path:
        raise ValueError('ns-3 path not found')

    agent = args['agentName']
    mempool_key = args.pop('mempoolKey')
    scenario = args.pop('scenario')
    seed = args.pop('seed')

    ns3_args = args
    ns3_args['RngRun'] = seed

    # set up the reward function
    reward_probs = np.asarray([args.pop('massive'), args.pop('throughput'), args.pop('urllc')])

    def normalize_rewards(env):
        fairness = 1 + FAIRNESS_SCALE * (env.fairness - 1)
        throughput = THROUGHPUT_SCALE * env.throughput / dataRate
        latency = 1e-3 * LATENCY_SCALE / env.latency

        rewards = np.asarray([fairness, throughput, latency])
        rewards = np.clip(rewards, 0, 1)
        rewards = REWARD_SCALE * np.dot(reward_probs, rewards)

        return rewards, REWARD_SCALE - rewards

    # set up the warmup function
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
        context = None
    elif agent == 'MLDR':
        rlib = RLib(
            agent_type=ThompsonSampling,
            agent_params={'n_arms': N_CW * N_RTS_CTS * N_AMPDU},
            no_ext_mode=True,
            logger_types=PlotsLogger,
            logger_params={'plots_dir': '.'},
            logger_sources='n_successful'
        )
        rlib.init(seed)
        context = np.ones(N_CW * N_RTS_CTS * N_AMPDU)
    else:
        raise ValueError('Invalid agent type')

    # set up the environment
    exp = Experiment(mempool_key, MEM_SIZE, scenario, ns3_path, using_waf=False)
    var = Ns3AIRL(MEMBLOCK_KEY, Env, Act)

    try:
        # run the experiment
        ns3_process = exp.run(setting=ns3_args, show_output=True)
        action = None

        while not var.isFinish():
            with var as data:
                if data is None:
                    break

                n_successful, n_failed = normalize_rewards(data.env)
                action = rlib.sample(
                    sample_observations={'context': context},
                    update_observations={'action': action, 'n_successful': n_successful, 'n_failed': n_failed, 'delta_time': ns3_args['interactionTime']}
                )
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
