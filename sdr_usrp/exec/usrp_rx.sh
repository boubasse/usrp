#!/bin/bash

#libdir=~/usrp/lib/loopProg/bin
killall -9 loopRead.run usrptx.run

SampleRate=1500000
CenterFrequency=1240000000
Gain=5

#unlink capture.iq
#mkfifo capture.iq -m666


../bin/usrprx.run -s $SampleRate -f $CenterFrequency -g $Gain
