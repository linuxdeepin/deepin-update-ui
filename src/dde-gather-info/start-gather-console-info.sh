#!/bin/bash  

random_seconds=$(( ( RANDOM % 600 ) + 60 ))
sleep $random_seconds

/usr/bin/lastore-tools gatherinfo
