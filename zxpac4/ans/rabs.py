# 
# (c) 2025-6 Jouni 'Mr.Spiv' Korhonen
#
# This example code implements streaming uABS binary encoder and decoder.
# The renormalization part has been 'optimized' for 8-bit architectures
#

import math

#
# Some technical and design choices:
#  - Maximum frequency is 256
#  - Adaptive modelling has been learning rate based on
#    https://fgiesen.wordpress.com/2015/05/26/models-for-adaptive-arithmetic-coding/
#  - In modelling only the probability of 1 is maintained and
#    uses 1 byte i.e. the range is [1..255] per context
#  - I also read Ferris' great blog
#    https://yupferris.github.io/blog/2019/02/11/rANS-on-6502.html
#  - Target decoder architecture is Z80
#





#F =   [3,3,2]                  # M = 8
#F =   [12288,12288,8192]       # M = 32768
F =   [24576,24576,16384]       # M = 65536

L_BYTE_BOUND = 1 << 14

# Propabiliry of 0.5
prop_of_1   = int(128)

# Max sum of freq, and propability of 1.0
M           = int(256)

# Throttle modem update rate
UPDATE_RATE = 4


#
def update_propability(prop,bit):
    if (bit == 0):
        update = (256 - prop) >> UPDATE_RATE
            
        if (update == 0):
            update = 1

        prop = prop + update

        if (prop > 255):
            prop = 255

    else:
        update = prop >> UPDATE_RATE

        if (update == 0):
            update = 1

        prop = prop - update

        if (prop == 0):
            prop = 1;

    return prop


def encode(out,state,bit):
    state_max = ((L_BYTE_BOUND // M) << 8) * Fi     # Becomes a shift if M is pow_of_2
    while (state >= state_max):
        print(f"  renorm 0x{state & 0xff:02x}")
        out.append(state & 0xff)
        state >>= 8

    # encode
    new_state = 0
    d = state // Fi
    r = state % Fi
    Ci = C[symbol]

    new_state = d * M + Ci + r
    return new_state

def decode(out,state):
    # decode
    d = state >> 8
    r = state & 255

    # s = freq_to_index(r)
    if (r < prop_of_1):
        s = 0
        Is = 0
    else:
        s = 1
        Is = prop_of_1

    # Fs = F[s]
    # Is = C[s]

    # If Fs is pow_of_2 then d*Fs becomes a d<<log2(Fs)
    # new_state = d * Fs + r - Is
    new_state = (d * prop_of_1) + r - Is  



    # renorm
    while (new_state < L_BYTE_BOUND):
        b = out.pop() & 0xff
        new_state = (new_state << 8) | b
        print(f"  renorm 0x{b:02x}")

    return s,new_state

def init_cum_freq(freq: []) -> []:
    cum_freq = []
    tmp_freq = 0;

    for n in freq:
        cum_freq.append(tmp_freq)
        tmp_freq += n

    cum_freq.append(tmp_freq)
    return cum_freq


def freq_to_index(r):
    symbol = 0

    while (r >= C[symbol+1]):
        symbol += 1

    return symbol


def get_M(freq_array):
    return sum(freq_array)



if (__name__ == "__main__"):

    S = [0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0]
    O = []
    out = []

    C = init_cum_freq(F)
    M = get_M(F)
    print(C)
    print(M)


    state = L_BYTE_BOUND
    print(S)

    for symbol in reversed(S):
        state = encode(out,state,symbol)
        #print(f"Input: {symbol} and state: 0x{state:x}");

    print(f"Final state: 0x{state:x}\n")
    print(out)

    O = []

    for i in range(S.__len__()):
        symbol,state = decode(out,state)
        #print(f"Output: {symbol} and state: 0x{state:x}");
        O.append(symbol)

    print(O)

