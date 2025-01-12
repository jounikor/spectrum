#!/usr/bin/python

import sys
import os

# 20250110 a sad update from python2 script to python3.. and the reason
#          why all this trouble is just that objcopy does not exist in
#          my MacOS 12.7.6.. and llvm-objcopy is not availbale because
#          Brew wants me to upgare my already OCLPed MacBook.. damnit.
#          After the Brew episode I just gave up with fancy build
#          system of binaries into the code..

class conput:
    def __init__(self):
        return

    def write( self,str ):
        sys.stdout.write( str )

    def close( self ):
        sys.stdout.flush()



def bin2c( lab, b, n, o ):
   
    ss = "unsigned char {:s}[{:d}] = {{\n".format(lab,n)
    o.write(ss.encode())

    i = 0

    while i < n:
        if i % 8 == 0:
            o.write("\t".encode())

        o.write("0x{:02x}".format(b[i]).encode())

        if i < n-1:
            if i % 8 == 7:
                o.write(",\n".encode())
            else:
                o.write(",".encode())

        i = i+1


    o.write("\n};\n".encode())


#
#

def main():
    if (len(sys.argv) < 3) or (len(sys.argv) > 4) :
        print("**Usage: {0} name infile [outfile]".format(sys.argv[0]))
        return

    #
    fi = open(sys.argv[2],"rb")
    fi.seek(0,os.SEEK_END)
    flen = fi.tell()
    fi.seek(0,os.SEEK_SET)

    # do not read attribute data, just gfx
    ib = bytearray(flen)
    fi.readinto(ib)
    fi.close()

    # check output method
    if len(sys.argv) == 4:
        fo = open(sys.argv[3],"wb")
    else:
        fo = conput()

    # do
    bin2c(sys.argv[1],ib,flen,fo)
    fo.close()


    # done
    return 0



#
#

if __name__ == "__main__":
    main()



