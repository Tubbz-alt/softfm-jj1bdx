#!/usr/bin/env python3
# Curve-fitting calculation script for sinc compensation

import math

def aperture(x):
    if x == 0.0:
        return 1.0
    else:
        return 2.0 / x * math.sin(x/2.0)

def mov1(x):
    y = math.sin(x/4.0);
    if y == 0.0:
        return 0.0
    else:
        return 0.5 * math.sin(x/2.0) / math.sin (x/4.0)

maxfreq = 480000

fitfactor = 0.095202571
sqsum_logratio = 0
for freq in range(50,54000,1000):
    theta = 2 * math.pi * freq / maxfreq;
    compensate = 1.0 / aperture(theta)
    # filter model: 
    # output[n] = input[n] * 1.1 -
    #   fitfactor * (input[n-1] + input[n]) / 2.0
    # ->
    # output[n] = input[n] * (1.1 - fitfactor / 2.0) -
    #             input[n-1] * fitfactor / 2.0
    fitlevel = 1.1 - (fitfactor * mov1(2 * theta))
    logratio = math.log10(compensate / fitlevel)
    sqsum_logratio += logratio * logratio
    output = "freq = " + str(freq);
    output += " compensate = " + str(compensate)
    output += " fitlevel = " + str(fitlevel)
    output += " logratio = " + str(logratio)
    print(output)

print("factor = " + str(fitfactor) + " sqsum_logratio = " + str(sqsum_logratio))

