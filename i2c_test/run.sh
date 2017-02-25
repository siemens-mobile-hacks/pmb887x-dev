#!/bin/bash
perl ../boot.pl --device="/dev/$(grep -r 'pl2303' /sys/class/tty/ttyUSB*/device/uevent -l | head -n1 | egrep -o 'ttyUSB[0-9]*')" --boot=i2c_test.bin --ign
