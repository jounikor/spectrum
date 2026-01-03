# 
# (c) 2025-6 Jouni 'Mr.Spiv' Korhonen
#
# This example code implements streaming rABS binary encoder and decoder.
# The renormalization part has been 'optimized' for 8-bit architectures
#
#
# Some technical and design choices:
#  - Adaptive modelling has a learning rate based on
#    https://fgiesen.wordpress.com/2015/05/26/models-for-adaptive-arithmetic-coding/
#  - When modelling probabilities the symbol frequencies/propabilities of 0
#    and 1 are maintained and a single symbol frequency/propability uses 1
#    byte i.e. the range is [1..255] per context.
#  - Total of symbol frequencies is 256 (for symbols 0 and 1).
#  - I also read Ferris' great blog
#    https://yupferris.github.io/blog/2019/02/11/rANS-on-6502.html
#  - Target decoder architecture is Z80
#
# How this rABS implementation works? First it is designed for end to start
# decompression. The end to start decompression is practical for inplace
# decompression where the compressed file and the decompressed file areas
# overlap ~entirely, which is a common requirement for low end platform
# "demo" compressors.
#
# Since the alphabet is only 0 and 1, we can cut corners on few things:
#  - We need to maintain only one symbol frequency/propability value for
#    "all" two symbols. Either the frequency/propability of 0 or 1, The
#    choise is up to you. I used the frequency/propability of 0.
#  - We do not need to calculate or maintain cumulative propabilities.
#    It is either 0 or the only maintained symbol frequency/propability.
#
# Set initial propability for 0 bit prop_of_0 to e.g. 0.5 (50%).
# Assume binary symbols:
#  S = [0,   1,   0,   1,   0,   0,   0,   0]
#       <----------------------------------- Update model from end to start.
#                                            Insert frquencies into array S. 
#  P = [128, 132, 136, 140, 144, 141, 145, 142] <- Example frequencies
#                                                  prop_of_0 is frequency/M
# Set intial state to L_BIT_LOW.
#  S = [...                                 ] 
#       -----------------------------------> Encode from start to end, and
#                                            pop the correcsponding symbol
#                                            propability from the array S.
#                                            
# After rABS encoding we have final state after the last symbol, whose
# propability is the known initial 0.5 (i.e. frequency 128).
# Now we can easily decode rABS output from end to start with a known final
# state, a known initial symbol frequency/propability, and update symbol
# propabilities dynamically as we decode...
#


# M must be a power of two.. and in this case also fixed to 256
M = 256
assert (M & (M - 1)) == 0, "M is not a power of 2"
assert M <= 256, "M must be 256"

# L can be selected so that we can check against a 16-bit register sign bit.
# The state must always reside between [L_BIT_LOW,0xffff] 
# The L_BITS defines how many bits are output/input at once.
# TODO: If 16-bit overflow can be used in case of L_BIT_LOW and state
#       maintenance? That would be good for 8-bit CPU targets.
L_BITS = 1
L_BITS_MASK = (1 << L_BITS) - 1
L_BIT_LOW = 0x10000 >> L_BITS
assert L_BITS < 8, "L_BITS must be less than 8"

# Initial propabiliry of 0.5
INIT_PROP_FOR_0 = 128
assert INIT_PROP_FOR_0 > 0, "INIT_PROP_FOR_0 must be greater than 1"
assert INIT_PROP_FOR_0 < 256, "INIT_PROP_FOR_0 must be less than 256"

# Throttle model update rate. Must be a power of two.
UPDATE_RATE = 32
assert (UPDATE_RATE & (UPDATE_RATE - 1)) == 0, "UPDATE_RATE is not a power of 2"
assert UPDATE_RATE < 256, "UPDATE_RATE must be less than 256"

# The model used here is "too simple" but serves for educational purposes.
# The model update rate/speed can be controlled with UPDATE_RATE.
# The symbol propability is between [1,255]
#
def update_propability(prop: int ,symbol: int ) -> int:
	if (symbol == 1):
		update = (M - prop) // UPDATE_RATE
            
		if (update == 0):
			update = 1

		prop = prop - update

		if (prop <= 0):
			prop = 1
	else:
		update = prop // UPDATE_RATE

		if (update == 0):
			update = 1

		prop = prop + update

		if (prop >= M):
			prop = M - 1;

	return prop

#
def encode(out: [],state: int, symbol: int, prop_of_0: int ) -> int:
	if (symbol == 0):
		Fi = prop_of_0
		Ci = 0
	else:
		Fi = M - prop_of_0
		Ci = prop_of_0
	
	# Fi = a symbol propability/frequency
	# Ci = cumulative propability/frequency for a synbol

	state_max = ((L_BIT_LOW // M) << L_BITS) * Fi
	while (state >= state_max):
		out.append(state & L_BITS_MASK)
		state >>= L_BITS

	new_state = ((state // Fi) * M) + Ci + (state % Fi)
	return new_state

#
def decode(out: [],state: int, prop_of_0: int) -> (int,int):
    # decode
    d = state // M
    r = state & (M - 1)

    if (r < prop_of_0):
        symbol = 0
        Fs = prop_of_0
        Is = 0
    else:
        symbol = 1
        Fs = M - prop_of_0
        Is = prop_of_0
    
    # Fs = F[s]
    # Is = C[s]
    #
    # Thinking Z80 implementation.. assume:
    #  HL = state
    #  D  = Fs
    #  E  = Is
    #
    # Then new_state = mul_8x8_to_16(H,D) + L - E
    new_state = (d * Fs) + r - Is  

    # renorm.. thinking Z80 implementation and assuming:
    #  L_BITS = 1
    #  HL = new_state
    #  A  = bit buffer
    # then the below renorm while loop could be something like:
    #
    #  _first_test:
    #           BIT    7,H
    #           JR NZ, _done
    #  _while:
    #           ADD    A,A   
    #           ADC    HL,HL
    #           JP P,  _while
    #  _done:
    #
    while (new_state < L_BIT_LOW):
        b = out.pop() & L_BITS_MASK
        new_state = (new_state << L_BITS) | b

    return symbol,new_state

#
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
		O.insert(0,symbol)
		prop_of_0 = update_propability(prop_of_0,symbol)

	print("Decoded")
	print(f"O:", O)
	print("Original input")
	print(f"S:", S)

#/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */
