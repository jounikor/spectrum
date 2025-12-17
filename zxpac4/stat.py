#
# This is for zxpac4c log output only
#

import matplotlib.pyplot as plt
import re
import sys

#

if (sys.argv.__len__() < 2):
	sys.exit("Usage: python3 " + sys.argv[0] + " zxpac4c-log-file")


with open(sys.argv[1]) as f:
    # O: Match, bits(1,1), tANS offset: 0x71, 7,      0 (   113),  0, k: 2, b: 0x00, tANS length:   2 (  2), 1, k: 1, b: 0x00 -> total 12	
    re_match = mo = re.compile(r"O: Match.*?[(](?P<offset>[ \d]+)[)].*?[(](?P<length>[ \d]+)[)]")

    stat = {}


    while True:
        s = f.readline()
		
        if (not s):
            break

        mm = re_match.match(s)
        if (mm == None):
            continue

        if (mm):
            print(mm.group("offset"),mm.group("length"))
			
            #offset = int(mm.group("offset")) & 0x7f
            #offset = int(int(mm.group("offset")) // 128)
            offset = int(int(mm.group("offset")) // 1)
            length = int(mm.group("length"))

            plt.plot(offset,length,"r+")


            #i = length & 0x7f
            stat[offset] = stat.get(offset,0) + 1
        else:
            continue

    sorted_stat = sorted(stat.items())
    keys,values = zip(*sorted_stat)
    #plt.bar(keys,values)
    #plt.bar(keys,values,color='skyblue', edgecolor='black')
    #plt.scatter(keys,values)
    plt.ylabel("Length")
    plt.xlabel("Offset")
    plt.grid(True)

    plt.show()

