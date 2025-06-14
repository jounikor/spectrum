# (c) 2025 Jouni 'Mr.Spiv' Korhonen
# 
# Some test code for tabled ANS encoder and decoder.
#

import math

#
#
#

class tANS(object):
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

    def spreadStep(self):
        return 5

    def debug(self,DEBUG=True):
        self.DEBUG = DEBUG


#
#
#

class tANS_encoder(tANS):
    #
    def __init__(self, M:int, Ls:[int],DEBUG=False):
        super().__init__(M,DEBUG)

        if (not self.scaleSymbolFreqs(Ls)):
            raise RuntimeError("**Error: scaling frequencies failed")

        self.buildEncodingTables()


    #
    def buildEncodingTables(self):
        self.L     = [0] * self.M                       # not needed
        self.symbol_last = [-1] * self.Ls.__len__()     # maybe needed..
        self.y     = [0] * self.M                       # not needed
        self.k     = [0] * self.M 
        self.next  = [[0] * self.M for tmp in range(self.Ls.__len__())]

        xp = 0

        for s in range(self.Ls.__len__()):
            c = self.Ls[s]

            for p in range(c,2*c):
                xp = (xp + self.spreadStep()) % self.M
                # L for illustration purposes
                # this is implicity the x value, no need for a table
                self.L[xp] = s
                # y is for illustartive purposes
                self.y[xp] = p
                
                
                # k and next tables for each symbol
                k_tmp = self.get_k(p,self.M)
                yp_tmp = p << k_tmp
                
                self.k[xp] = k_tmp

                for yp_pos in range(yp_tmp,yp_tmp+(1<<k_tmp)):
                    if (self.DEBUG):
                        print("s",s,"p",p,"xp",xp,"k_tmp",k_tmp,"yp_tmp",yp_tmp,"y_pos",yp_pos)
                    self.next[s][yp_pos % self.M] = xp #+ self.M
                    
            self.symbol_last[s] = xp #+ self.M



    def scaleSymbolFreqs(self, Ls):
        L = sum(Ls)
        R = int(math.floor(math.log2(L)))

        if (L != self.M):
            # not M i.e. a power of two.. scale up or down
            
            if (self.DEBUG):
                print(f"DEBUG> new_L: {self.M}, R: {R}, L: {L}")

            # https://stackoverflow.com/questions/31121591/normalizing-integers
            # The last remined gets always added to the last item in the list..
            rem = 0
            ufl = 0
            for i in range(Ls.__len__()):
                count = Ls[i] * self.M + rem
                tmp = int(count / L)
                
                if (tmp == 0):
                    if (self.DEBUG):
                        print(f"DEBUG: Ls[{i}] would become 0");
                    ufl += 1
                    tmp = 1
                
                Ls[i] = tmp
                rem = count % L

            if (self.DEBUG):
                print(f"Number of underflows is {ufl}")

            i = Ls.__len__() - 1
            while (ufl > 0 and i > 0):
                if (Ls[i] > 1):
                    Ls[i] -= 1
                    ufl -= 1
                i -= 1

            L = sum(Ls)
            R = int(math.log2(L))
            
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
            self.state = self.symbol_last[x]
            return 0,0

        if (self.DEBUG):
            print(f"symbol: {s}, state: {self.state}, ",end="")

        #k = self.k[self.state % self.M]
        k = self.k[self.state]
        b = self.state & ((1 << k) - 1)
        #self.state = self.next[s][self.state % self.M]
        self.state = self.next[s][self.state]

        if (self.DEBUG):
            print(f"new state: {self.state}")
        return k,b

#
#
#
#

class tANS_decoder(tANS):
    #
    def __init__(self, M:int, Ls:[int],DEBUG=False):
        super().__init__(M,DEBUG)
        self.buildDecodingTables()

    def buildDecodingTables(self):
        # Only y and k tables needed..
        self.y     = [0] * self.M                       # not needed
        self.k     = [0] * self.M 

        xp = 0

        for s in range(Ls.__len__()):
            c = Ls[s]

            for p in range(c,2*c):
                xp = (xp + self.spreadStep()) % self.M
                self.y[xp] = p
                
                # k and next tables for each symbol
                k_tmp = self.get_k(p,self.M)
                self.k[xp] = k_tmp

    def init_decoder(self,state):
        self.state = state


if (__name__ == "__main__"):

    #S = [0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0]
    S = [0,2,1,2,0,1,1,2,1,2,1,1]
    out = []

    LLs = [2,6,4]

    tans = tANS_encoder(16,LLs,True)
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

    for symbol in reversed(S):
        k,b = tans.encode(symbol)
        out.append((k,b))

    state = tans.done_encoder()

    print(f"Final state: {state}\n")
    print(out)

    Ls = tans.get_scaled_Ls()

    O = []
    tans = tANS_decoder(16,Ls,True)
    tans.init_decoder(state)
    print(tans.y)
    print(tans.k)

    for i in range(S.__len__()):
        pass
        #symbol = decode()
        #print(f"Output: {symbol} and state: 0x{state:x}");
        #O.append(symbol)

    print(O)

