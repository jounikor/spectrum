#
# (c) 2025-06-15 Jouni 'Mr.Spiv' Korhonen
# 
# Some test code for tabled ANS encoder and decoder.. for self
# learnign and education purposes. Treat the code accordingly ;)
#
# The following article provided invaluable help. IMHO one of the
# best 'hands-on" tutorial for understanding rANS and tANS.
# https://medium.com/@bredelet/understanding-ans-coding-through-examples-d1bebfc7e076
#

import math

#
# The base class for both tANS encoder and decoder subclasses. The base
# class implements the common functions used by encoder and decoder.
#
class tANS(object):
    #
    # INITIAL_STEP and SPREAD_STEP determine how symbols are spread into
    # the L array, which eventually defines the state to the symbol mapping.
    # The "better" spreading function the better compression they say ;)
    #
    INITIAL_STATE = 3
    SPREAD_STEP = 5

    def __init__(self, M, DEBUG=False):
        if (M & (M - 1)):
            raise ValueError("**Error: M is not a power of 2")

        self.DEBUG = DEBUG
        self.M = M

    def get_k(self,base,threshold):
        k = 1

        while ((base << k) < threshold):
            k += 1

        return k

    def spreadFunc(self,x):
        # Cannot go simpler than this ;) The key point is that after M 
        # iterations all M slots in L are visited exactly once.
        return (x + self.SPREAD_STEP) % self.M

    def debug(self,DEBUG=True):
        self.DEBUG = DEBUG

#
# The encoder class, which implements three fundamental functions:
# 1) Scaling the input symbol frequencies to a desired sum that is
#    also a power of two.
# 2) Building the encoding tables for a completely table driver
#    encoding of symbols.
# 3) Encoding a symbol, next state computation and outputting the
#    additional bits required by the decoder to reconstruct the
#    state for a given symbol.
#
class tANS_encoder(tANS):
    #
    def __init__(self, M:int, Ls:[int],DEBUG=False):
        super().__init__(M,DEBUG)

        if (not self.scaleSymbolFreqs(Ls)):
            raise RuntimeError("**Error: scaling frequencies failed")

        self.buildEncodingTables()

    def buildEncodingTables(self):
        self.L     = [0] * self.M                       # Not needed but used for
                                                        # debugging purposes.
        self.symbol_last = [-1] * self.Ls.__len__()     # Used to select the 
                            # Advance to the next state                                            # initial state.
        self.y     = [0] * self.M                       # Not needed but used for
                                                        # debugging purposes.
        self.k     = [[0] * self.M for tmp in range(self.Ls.__len__())] 
        self.next  = [[0] * self.M for tmp in range(self.Ls.__len__())]

        # The initial state to start with..
        xp = self.INITIAL_STATE

        for s in range(self.Ls.__len__()):
            c = self.Ls[s]

            for p in range(c,2*c):
                # Note that we make the table indices reside between [0..L),
                # since values within [L..2L) % M is within [0..M). This is 
                # a simple algorithmic optimization to avoid extra index
                # adjusting during decoding..
               
                # L is for illustration purposes
                self.L[xp] = s
                
                # y is for illustration purposes
                self.y[xp] = p
                
                # Get the k's for the given symbol..
                k_tmp = self.get_k(p,self.M)
                yp_tmp = p << k_tmp
                
                for yp_pos in range(yp_tmp,yp_tmp+(1<<k_tmp)):
                    if (self.DEBUG):
                        print("s",s,"p",p,"xp",xp,"k_tmp",k_tmp,"yp_tmp",yp_tmp,"y_pos",yp_pos)
                    
                    # next table for each symbol.. this array will explode in size when the
                    # symbol set gets bigger.
                    self.next[s][yp_pos % self.M] = xp #+ self.M
                    
                    # k table for each symbol.. this array will explode in size when the
                    # symbol set gets bigger.
                    self.k[s][yp_pos % self.M] = k_tmp
                
                # advance to the next state..
                last_xp = xp
                xp = self.spreadFunc(xp)
               
            # Record the final state for the symbol. One of these will be used for the
            # initial state when starting encoding.
            self.symbol_last[s] = last_xp

    def scaleSymbolFreqs(self, Ls):
        L = sum(Ls)

        if (L != self.M):
            # not M i.e. a power of two.. scale up or down
            
            if (self.DEBUG):
                print(f"DEBUG> new_L: {self.M}, L: {L}")

            # https://stackoverflow.com/questions/31121591/normalizing-integers
            # The last reminder gets always added to the last item in the list..
            rem = 0
            ufl = 0
            for i in range(Ls.__len__()):
                # Skip symbol if its count is zero
                if (Ls[i] == 0):
                    continue

                count = Ls[i] * self.M + rem
                tmp = int(count / L)
                
                # Do we have an underflow..?
                if (tmp == 0):
                    if (self.DEBUG):
                        print(f"DEBUG: Ls[{i}] would become 0");
                    ufl += 1
                    tmp = 1
                
                Ls[i] = tmp
                rem = count % L

            if (self.DEBUG):
                print(f"Number of underflows is {ufl}")

            # My addition to handle underflows..
            i = Ls.__len__() - 1
            while (ufl > 0 and i > 0):
                if (Ls[i] > 1):
                    Ls[i] -= 1
                    ufl -= 1
                i -= 1

            L = sum(Ls)
            
            if (self.DEBUG):
                print("DEBUG> New Ls -> ",Ls)

        if (L != self.M):
            if (self.DEBUG):
                print(f"DEBUG: M ({self.M}) != L ({L}) ")
            return False

        self.Ls = Ls
        self.L = L
        return True

    def get_scaled_Ls(self):
        return self.Ls

    def init_encoder(self):
        self.state = None

    def done_encoder(self):
        return self.state

    def encode(self,s):
        if (self.state == None):
            self.state = self.symbol_last[s]
            
            if (self.DEBUG):
                print(f"symbol: {s}, initial state: {self.state}, ")
            
            return 0,0

        if (self.DEBUG):
            print(f"symbol: {s}, state: {self.state}, ",end="")

        k = self.k[s][self.state]
        b = self.state & ((1 << k) - 1)
        self.state = self.next[s][self.state]

        if (self.DEBUG):
            print(f"new state: {self.state}, k: {k}, b: {b}")
        return k,b

#
#
#
#

class tANS_decoder(tANS):
    #
    def __init__(self, Ls:[int],DEBUG=False):
        super().__init__(sum(Ls),DEBUG)
        self.buildDecodingTables()

    def buildDecodingTables(self):
        # Only L, and y tables needed..
        # When implementing the final output file you need the scaled symbol
        # frequencies in Ls (with a total sum of M (a power of two value)).
        # The Ls table must have frequency 0 for non-used symbols. Note, the
        # number of symbols in Ls does not need to be a power of two.
        #
        # For example, if Ls has 12 symbols but the sum(Ls) is 32 then you
        # need 2x 32 bytes of RAM to build all 3 decoding tables. As long
        # as the M <= 256 then table entries can be 1 unsigned byte each.
        #
        self.L = [0] * self.M   # state to symbol mapping table
        self.y = [0] * self.M   # y values for each state

        xp = self.INITIAL_STATE

        for s in range(Ls.__len__()):
            c = Ls[s]

            for p in range(c,2*c):
                self.y[xp] = p
                self.L[xp] = s

                # Advance to the next state
                xp = self.spreadFunc(xp)

    def init_decoder(self,state,pos):
        self.state = state
        self.rpos = pos

    def decode(self,file):
        s = self.L[self.state]
        if (self.rpos >= 0):
            self.rpos -= 1
            k,b = file[self.rpos]
            y = self.y[self.state] << 1
            
            # k would be the count how many bots to read from input
            k = 1

            while (y < self.M):
                y = y << 1
                k = k + 1
            
            self.state = (y + b) % self.M
            #self.state = ((self.y[self.state] << k) + b) % self.M
        return s

#
#
#
#

if (__name__ == "__main__"):

    S = [0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0,5,5,5,5,5,5,5,5,5,5,5,5,5,6]
    #S = [0,2,1,2,0,1,1,2,1,2,1,1]
    out = []

    #LLs = [12,2,2,0]
    LLs = [4,3,1,0,0,5,3,1]
    #LLs = [2,6,4,0]
    #LLs = [3,3,2]

    tans = tANS_encoder(32,LLs,True)
    print("L:")
    print(tans.L)
    print("y:")
    print(tans.y)
    print("k:")
    print(tans.k)
    print("next:")
    print(tans.next)
    print("symbol_last:")
    print(tans.symbol_last)
    x = 0

    tans.init_encoder()

    # This encodes from start to end, which is OK, since in a real
    # application we decode from end to beginning in any case.
    for symbol in S:
        k,b = tans.encode(symbol)
        if (k > 0):
            out.append((k,b))

    state = tans.done_encoder()

    print(f"Final state: {state}\n")
    print(out)

    Ls = tans.get_scaled_Ls()

    O = []
    tans = tANS_decoder(Ls,True)
    tans.init_decoder(state,out.__len__())
    print("L table for state to symbol mapping")
    print(tans.L)
    print("y:")
    print(tans.y)

    for i in range(S.__len__()):
        symbol = tans.decode(out)
        print(f"Output: {symbol} and state: {tans.state}");
        O.insert(0,symbol)

    print("Original:")
    print(S)
    print("Decoded:")
    print(O)

