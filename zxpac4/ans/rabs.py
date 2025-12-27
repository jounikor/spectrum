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

L_BIT_LOW = 0x8000

# Propabiliry of 0.5
INIT_PROP_FOR_0 = int(10)


# Throttle model update rate
UPDATE_RATE = 4

#
def update_propability(prop,bit):
    print(f"update: {prop}, {bit}")
    if (bit == 1):
        update = (256 - prop) >> UPDATE_RATE
            
        if (update == 0):
            update = 1

        prop = prop - update

        if (prop <= 0):
            prop = 1

    else:
        update = prop >> UPDATE_RATE

        if (update == 0):
            update = 1

        prop = prop + update

        if (prop > 255):
            prop = 255;


    print(f"  updated: {update}, {prop}")
    return prop


def encode(out,state,bit):
	if (bit == 0):
		Fi = prop_of_0
		Ci = 0
	else:
		Fi = 256 - prop_of_0
		Ci = prop_of_0

	state_max = ((L_BIT_LOW >> 8) << 1) * Fi
	while (state >= state_max):
		print(f"  renorm 0x{state:04x} - {state & 1:d} ->",end=" ")
		out.append(state & 0x01)
		state >>= 1
		print(f"0x{state:04x}")

	new_state = ((state // Fi) << 8) + Ci + (state % Fi)
	return new_state



def decode(out,state):
    # decode
    d = state >> 8
    r = state & 255

    # s = freq_to_index(r)
    if (r < prop_of_0):
        s = 0
        Fs = prop_of_0
        Is = 0
    else:
        s = 1
        Fs = 256 - prop_of_0
        Is = prop_of_0

    # Fs = F[s]
    # Is = C[s]

    # If Fs is pow_of_2 then d*Fs becomes a d<<log2(Fs)
    # new_state = d * Fs + r - Is
    new_state = (d * Fs) + r - Is  

    # renorm
    while (new_state < L_BIT_LOW):
        b = out.pop() & 0x01
        print(f"  renorm 0x{new_state:04x} + {b:d} ->", end=" ")
        new_state = (new_state << 1) | b
        print(f"0x{new_state:04x}")

    return s,new_state

if (__name__ == "__main__"):
    S = [0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,0,0,0]
    O = []
    out = []

	# initialize
    state = L_BIT_LOW
    global prop_of_0
    prop_of_0 = INIT_PROP_FOR_0

    for symbol in reversed(S):
        state = encode(out,state,symbol)
        print(f"Input: {symbol}, state: 0x{state:x}, prop_of_0: 0x{prop_of_0:x}");
        #prop_of_0 = update_propability(prop_of_0,symbol)


    print(f"Final state: 0x{state:x}, final prop_of_0: 0x{prop_of_0:x}\n")
    print(out)

    O = []
    prop_of_0 = INIT_PROP_FOR_0

    for i in range(S.__len__()):
        symbol,state = decode(out,state)
        print(f"Output: {symbol} and state: 0x{state:x}");
        O.append(symbol)
        #prop_of_0 = update_propability(prop_of_0,symbol)

    print(O)

