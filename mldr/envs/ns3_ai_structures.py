from ctypes import *


# ns3-ai environment structure
class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('fairness', c_double),
        ('latency', c_double),
        ('plr', c_double),
        ('throughput', c_double),
        ('time', c_double)
    ]


# ns3-ai action structure
class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('cw', c_int),
        ('rts_cts', c_bool),
        ('ampdu', c_bool),
        ('end_warmup', c_bool)
    ]
