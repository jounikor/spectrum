#
#
#
#

import math

#F =   [3,3,2]                  # M = 8
F =   [12288,12288,8192]       # M = 32768
#F =   [24576,24576,16384]       # M = 65536

L_BYTE_BOUND = 1 << 23


def encode(out,state,symbol):
    M = get_M(F)
    Fi = F[symbol]

    # renorm
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
    new_state = 0
    M = get_M(F)
    d = state // M
    r = state % M
    s = freq_to_index(r)
    Fs = F[s]
    Is = C[s]

    # If Fs is pow_of_2 then d*Fs becomes a d<<log2(Fs)
    new_state = d * Fs + r - Is
    
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


    


def quantisize(F):

    O = [0] * F.__len__()
    P = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]

    for value in F:
        p = int(math.log2(value))    

        distLo = int(value - 2**p)
        distHi = int(2**(p+1) - value)

        print(f"{2**p} <= {value} < {2**(p+1)}")
        print(f" >> {p}, {distLo},{distHi}")

        if (distLo == distHi):
            p += 1
        elif (distLo > distHi):
            p += 1

        if (p > 15):
            p = 15

        P[p] += 1

    print(f">>> {P}")

    return O


if (__name__ == "__main__"):

    S = [0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0]
    O = []
    out = []

    C = init_cum_freq(F)
    M = get_M(F)
    print(C)
    print(M)


    #quantisize(F)


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

