# Witmotion WT901C485 ROS2 Driver

## Description

This is a ros2 driver for WT901C485. \
I check this program only in ROS2 humble with ubuntu22.04.


## Installation

### install serial_connect library

```
git clone -b v1.2 https://github.com/k-trash/serial_connect
cd serial_connect
mkdir build
cd build
cmake ..
make
sudo make install
```

### add user to dialout group

```
sudo gpasswd -a $USERNAME dialout
```
then reboot 


### clone repository

```
cd <Your-ROS2-WS>/src
git clone https://github.com/k-trash/witmotion_wt901
cd ..
colcon build --symlink-install --packages-select witmotion_wt901
```

## Usage

### prepare sensor

Make sure your WT901C485 communicate at 115200 baudrate. \
You can modify the baudrate with [windows application](https://www.wit-motion.com/searchq.html
) which Witmotion provides.

### run ros2 node

```
cd <Your-ROS2-WS>
source install/setup.bash
ros2 run witmotion_wt901 witmotion_wt901_node
```
then you can get /imu_data with sensor_msgs::msg::Imu message.
