#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/config/lib/:/customer/lib
export ALSA_CONFIG_DIR=/customer/res/alsa
(./amixer cset name='AMP_CTL' 1)&
wait
chmod 777 mp3Player
./mp3Player -i /customer/res/start.mp3 -v  50
./mp3Player -i /customer/res/done.mp3  -v 50
./mp3Player -i /customer/res/fail.mp3  -v 50
./mp3Player -i /customer/res/Q001.mp3  -v 50
./mp3Player -i /customer/res/start.mp3 -v  50
./mp3Player -i /customer/res/done.mp3  -v 50
./mp3Player -i /customer/res/fail.mp3  -v 50
./mp3Player -i /customer/res/Q001.mp3  -v 50
(./amixer cset name='AMP_CTL' 0)&
wait