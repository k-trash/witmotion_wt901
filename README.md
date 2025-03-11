# Witmotion WT901C485 ROS2 Driver

## Description

This is a ros2 driver for WT901C485. \
I check this program only in ROS2 humble with ubuntu22.04.


## Installation

### install serial_connect library

```
git clone -b release-v1.3 https://github.com/k-trash/serial_connect
cd serial_connect
mkdir build && cd build
cmake .. && make
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

Make sure your WT901C485 communicate at 115200bps baudrate. \
You can modify the baudrate with [windows application](https://www.wit-motion.com/searchq.html
) which Witmotion provides.

### run ros2 node

```
cd <Your-ROS2-WS>
source install/setup.bash
ros2 run witmotion_wt901 witmotion_wt901_node
```
you can also use launch file.
```
ros2 launch witmotion_wt901 witmotion_wt901.launch.xml
```

### parameters

| param name | default value | description |
| :--- | :--- | :--- |
| port | /dev/ttyUSB0 | IMU port |
| imu_topic | imu/data_raw | IMU topic name |
| mag_topic | mag/data_raw | Magnetic Field Sensor topic name |
| imu_frame_id | imu_link | link name of IMU topic |
| imu_freq | 100 | frequency of IMU [Hz] (max 100) |
| warn_freq | 5 | get warning and auto reconnection frequency [Hz] |

### output

|topic name|topic type|
| :--- | :--- |
|/imu/data_raw|sensor_msgs::msg::Imu|
|/mag/data_raw|sensor_msgs::msg::MagneticField|
