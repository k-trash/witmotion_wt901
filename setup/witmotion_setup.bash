#!/bin/bash

cp -p 50-witmotion-imu.rules /etc/udev/rules.d
udevadm trigger
gpasswd -a $USERNAME dialout