#!/bin/bash

loopdir=~/usrp/lib/loopProg/bin
scopedir=~/usrp/lib/kisspectrumFolder/bin
killall -9 loopRead.run usrptx.run

SampleRate=1500000
CenterFrequency=1240000000
Gain=5

sudo unlink capture.iq
sudo unlink spectrum.iq

sudo mkfifo capture.iq -m666
sudo mkfifo spectrum.iq -m666

$loopdir/loopRead.run -i capture_NR_i16.bin -r i16 -w i16 -o capture.iq &
../bin/usrptxrx.run -s $SampleRate -f $CenterFrequency -g $Gain -i capture.iq -o spectrum.iq &
$scopedir/kisspectrum.run -s $SampleRate -r 25 -t i16 -i spectrum.iq
