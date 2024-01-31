#!/bin/bash

killall -9 loopRead.run usrptx.run

SampleRate=1500000
CenterFrequency=1240000000
Gain=5

#unlink capture.iq
#mkfifo capture.iq -m666



./loopRead.run -i capture_NR_i16.bin -r i16 -w i16 | ./usrptx.run -s $SampleRate -f $CenterFrequency -g $Gain

