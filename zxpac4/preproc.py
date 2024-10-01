# (c) Jouni 'mr.spiv/scoopex' Korhonen
# v0.1 3-Aug-2024
#
# Translations for foreground color codes:
#
#  Ctrl-A + K   -> 0x02
#  Ctrl-A + R   -> 0x03
#  Ctrl-A + G   -> 0x04 
#  Ctrl-A + Y   -> 0x05
#  Ctrl-A + B   -> 0x06
#  Ctrl-A + M   -> 0x07
#  Ctrl-A + C   -> 0x08
#  Ctrl-A + W   -> 0x09
#
# Translatons for attributes and controls
#
#  Ctrl-A + N   -> 0x0e
#  Ctrl-A + H   -> 0x0f
#  Ctrl-A + Z   -> 0x10
#

import os
import sys
import argparse

prs = argparse.ArgumentParser()
prs.add_argument("input_file",metavar="input_file",type=argparse.FileType('r+b'),help="Input ASCII text file with ANSI codes")
prs.add_argument("output_file",metavar="output_file",type=argparse.FileType('w+b'),default=sys.stdout,help="Output preprocessed file")
prs.add_argument("--debug","-d",dest="debug",action="store_true",default=False,help="Show debug output")
prs.add_argument("--rem-cr","-c",dest="rem_cr",action="store_true",default=False,help="Remove Carriege Returns ('\\r')")
prs.add_argument("--rem-bg","-b",dest="rem_bg",action="store_true",default=False,help="Remove Background CTRL-A codes")
prs.add_argument("--expand","-e",dest="expand",action="store_true",default=False,help="Expand cursor right to spaces")
prs.add_argument("--translate-fg","-t",dest="translate_fg",action="store_true",default=False,help="Translate Foreground CTRL-A + codes to compact form")
prs.add_argument("--translate","-r",dest="translate",action="store_true",default=False,help="Translate CTRL-A + 'NHZ' to compact form")
args = prs.parse_args()


outlen = 0
origlen = 0


if __name__ == "__main__":
    while (b := args.input_file.read(1)):
        origlen += 1
       
        if (b >= b'\x80'):
            print(f"**File '{os.path.basename(args.input_file.name)}' contains illegal character {b} at {origlen-1}")
            args.output_file.write(b)
            outlen += 1
            continue

        if (args.rem_cr and b == b'\r'):
            # just consume '\r'
            continue
        if (b == b'\x01'):
            if (b := args.input_file.read(1)):
                origlen += 1

                if (args.expand and b >= b'\x80'):
                    spaces = b' ' * (int.from_bytes(b)-127)
                    args.output_file.write(spaces)
                    outlen += spaces.__len__()
                elif (args.translate_fg and b in b"KRGYBMCW"):
                    # map foreground colors between 2 and 9
                    args.output_file.write((b"KRGYBMCW".index(b)+2).to_bytes())
                    outlen += 1
                elif (args.rem_bg and b in b"01234567"):
                    # just remove background color codes 
                    continue
                elif (args.translate and b in b"NHZ"):
                    # map Ctrl-A + H/N/Z between 14 and 31
                    args.output_file.write((b"HNZ".index(b)+14).to_bytes())
                    outlen += 1
                else:
                    args.output_file.write(int(0x01).to_bytes())
                    args.output_file.write(b)
                    outlen += 1
                    
                    if (args.debug):
                        print(f"Not processed Ctrl-A + {b}")
            else:
                print("**Error: reading failed")
                sys.exit(1)
        else:
            args.output_file.write(b)
            outlen += 1

    if (args.debug):
        print(f"File '{os.path.basename(args.input_file.name)}' in {origlen} out {outlen}")

    sys.exit(0)
