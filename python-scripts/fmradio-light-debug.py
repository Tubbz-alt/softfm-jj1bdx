#!/usr/bin/env python3
# Simple caller for softfm
import sys
import subprocess
import signal

argvs = sys.argv
argc = len(argvs)

def signal_handler(signal, frame):
    print('Terminated by CTRL/C')
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

if argc != 2:
    print('Usage: ', argvs[0], '<frequency in MHz>\n')
    quit()
freq = float(argvs[1]) * 1000000
command = "softfm -f " + str(freq) + " -g 8.7 -s 240000 -r44100 -b 2.0 -R - | " + \
          "play -t raw -esigned-integer -b16 -r 44100 -c 2 -"
subprocess.run(command, shell=True, check=True)
