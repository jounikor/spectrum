# 
# (c) 2025-6 Jouni 'Mr.Spiv' Korhonen
#
# This example code implements streaming urBS binary encoder and decoder.
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

# M must be a power of two.. and in this case also fixed to 256
M = 256

# L is selected so that we can check against a 16 bit register sign bit
L_BIT_LOW = 0x8000

# Initial propabiliry of 0.5
INIT_PROP_FOR_0 = int(128)

# Throttle model update rate
UPDATE_RATE = 4

#
def update_propability(prop: int ,bit: int ) -> int:
	#print(f"update: {prop}, {bit}")
	if (bit == 1):
		update = (M - prop) >> UPDATE_RATE
            
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

		if (prop >= M):
			prop = M - 1;

	#print(f"  updated: {update}, {prop}")
	return prop

#
def encode(out: [],state: int,bit: int,prop_of_0: int ) -> int:

	if (bit == 0):
		Fi = prop_of_0
		Ci = 0
	else:
		Fi = M - prop_of_0
		Ci = prop_of_0
	
	state_max = ((L_BIT_LOW // M) << 1) * Fi
	while (state >= state_max):
		out.append(state & 0x01)
		state >>= 1

	new_state = ((state // Fi) * M) + Ci + (state % Fi)
	return new_state

#
def decode(out: [],state: int,prop_of_0: int) -> (int,int):
    # decode
    d = state // M
    r = state & (M - 1)

    # s = freq_to_index(r)
    if (r < prop_of_0):
        s = 0
        Fs = prop_of_0
        Is = 0
    else:
        s = 1
        Fs = M - prop_of_0
        Is = prop_of_0
    
    # Fs = F[s]
    # Is = C[s]

    # If Fs is pow_of_2 then d*Fs becomes a d<<log2(Fs)
    # new_state = d * Fs + r - Is
    new_state = (d * Fs) + r - Is  

    # renorm
    while (new_state < L_BIT_LOW):
        b = out.pop() & 0x01
        new_state = (new_state << 1) | b

    return s,new_state

if (__name__ == "__main__"):
	S = [0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,0,0,0]
	O = []
	out = []

	# initialize
	state = L_BIT_LOW

	# Calculate propabilities.. these are done backwards since we will decode
	# ANS encoded file from last to first symbol..
	prop_of_0 = INIT_PROP_FOR_0
	props = []

	for symbol in reversed(S):
		props.append(prop_of_0)
		prop_of_0 = update_propability(prop_of_0,symbol)

	print("Reversed propabilities:")
	print(props)

	# Encode from start to end using reversed propabilities i.e. pop the last
	# propabuility and encode the first symbol using it.

	for symbol in S:
		prop_of_0 = props.pop()
		state = encode(out,state,symbol,prop_of_0)

	print(f"Final state: 0x{state:x}")
	print(f"o:",out)

	# Decode from end to start.. state is the final and propabiloity we
	# will update while decoding

	prop_of_0 = INIT_PROP_FOR_0
	O = []

	for i in range(S.__len__()):
		symbol,state = decode(out,state,prop_of_0)
		O.append(symbol)
		prop_of_0 = update_propability(prop_of_0,symbol)

	print(f"O:", [o for o in reversed(O) if True])
	print(f"S:", S)

#/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */
