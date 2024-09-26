#include <cstdlib>
#include <cmath>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <geometry_msgs/msg/vector3.hpp>

#include <serial_connect/serial_connect.hpp>

void serialCallback(int32_t signal_);
void timerCallback(void);
void accelCalibration(void);
uint16_t getCrc(uint8_t *datas_, uint8_t size_);

rclcpp::Node::SharedPtr node;
rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub;
rclcpp::Publisher<sensor_msgs::msg::MagneticField>::SharedPtr mag_pub;
rclcpp::TimerBase::SharedPtr timer;

SerialConnect serial;

const double acc_range = 16.0f;
const double gyr_range = 2000.0f;
const double mag_range = 1.0f;
const double ang_range = 180.0f;

int main(int argc, char *argv[]){
	rclcpp::init(argc, argv);
	node = rclcpp::Node::make_shared("imu_node");

	node->declare_parameter<std::string>("port", "/dev/ttyUSB0");
	node->declare_parameter<std::string>("imu_topic", "imu/data_raw");
	node->declare_parameter<std::string>("mag_topic", "mag/data_raw");
	node->declare_parameter<std::string>("imu_frame_id", "imu_link");
	node->declare_parameter<int64_t>("imu_freq", 10);

	imu_pub = node->create_publisher<sensor_msgs::msg::Imu>(node->get_parameter("imu_topic").as_string(), 10);
	mag_pub = node->create_publisher<sensor_msgs::msg::MagneticField>(node->get_parameter("mag_topic").as_string(), 10);
	timer = node->create_wall_timer(std::chrono::milliseconds(1000/node->get_parameter("imu_freq").as_int()), &timerCallback);

	serial.setSerial(node->get_parameter("port").as_string(), B115200, true);
	serial.openSerial();

	accelCalibration();

	RCLCPP_INFO(node->get_logger(), "Accelaration calibration finished");

	serial.setInterrupt(&serialCallback);		//set uart receive interruption

	rclcpp::spin(node);

	rclcpp::shutdown();

	serial.closeSerial();

	return 0;
}

void serialCallback(int32_t signal_){
	sensor_msgs::msg::Imu imu_data;
	sensor_msgs::msg::MagneticField mag_data;
	geometry_msgs::msg::Vector3 rpy;
	tf2::Quaternion quat;
	int res = 0;
	uint16_t crc_code = 0u;

	res = serial.readSerial();

	RCLCPP_DEBUG(node->get_logger(), "serial interrupted %i", res);

	if(serial.recv_data[0] == 0x50 and res == serial.recv_data[2]+5 and serial.recv_data[1] == 0x03){			//check data length
		crc_code = getCrc(serial.recv_data, res-2);
		if(crc_code != ((serial.recv_data[res-2] << 8) | serial.recv_data[res-1])){
			RCLCPP_WARN(node->get_logger(), "receive crc incorrect");
			return;
		}
		imu_data.header.frame_id = node->get_parameter("imu_frame_id").as_string();
		imu_data.header.stamp = node->get_clock()->now();
		mag_data.header.frame_id = node->get_parameter("imu_frame_id").as_string();
		mag_data.header.stamp = node->get_clock()->now();
		imu_data.linear_acceleration.x = static_cast<int16_t>((serial.recv_data[3] << 8) | serial.recv_data[4]) / 32768.0f * acc_range * 9.8f;
		imu_data.linear_acceleration.y = static_cast<int16_t>((serial.recv_data[5] << 8) | serial.recv_data[6]) / 32768.0f * acc_range * 9.8f;
		imu_data.linear_acceleration.z = static_cast<int16_t>((serial.recv_data[7] << 8) | serial.recv_data[8]) / 32768.0f * acc_range * 9.8f;
		imu_data.angular_velocity.x    = static_cast<int16_t>((serial.recv_data[9] << 8) | serial.recv_data[10]) / 32768.0f * gyr_range;
		imu_data.angular_velocity.y    = static_cast<int16_t>((serial.recv_data[11] << 8) | serial.recv_data[12]) / 32768.0f * gyr_range;
		imu_data.angular_velocity.z    = static_cast<int16_t>((serial.recv_data[13] << 8) | serial.recv_data[14]) / 32768.0f * gyr_range;
		mag_data.magnetic_field.x      = static_cast<int16_t>((serial.recv_data[15] << 8) | serial.recv_data[16]) / 32768.0f * mag_range;
		mag_data.magnetic_field.y      = static_cast<int16_t>((serial.recv_data[17] << 8) | serial.recv_data[18]) / 32768.0f * mag_range;
		mag_data.magnetic_field.z      = static_cast<int16_t>((serial.recv_data[19] << 8) | serial.recv_data[20]) / 32768.0f * mag_range;
		rpy.x = static_cast<int16_t>((serial.recv_data[21] << 8) | serial.recv_data[22]) / 32768.0f * ang_range * M_PI / 180.0f;
		rpy.y = static_cast<int16_t>((serial.recv_data[23] << 8) | serial.recv_data[24]) / 32768.0f * ang_range * M_PI / 180.0f;
		rpy.z = static_cast<int16_t>((serial.recv_data[25] << 8) | serial.recv_data[26]) / 32768.0f * ang_range * M_PI / 180.0f;

		quat.setRPY(rpy.x, rpy.y, rpy.z);			//convert from rpy to quaternion

		imu_data.orientation.x = quat.x();
		imu_data.orientation.y = quat.y();
		imu_data.orientation.z = quat.z();
		imu_data.orientation.w = quat.w();

		imu_pub->publish(imu_data);
		mag_pub->publish(mag_data);
	}
}

void timerCallback(void){
	uint8_t send_data[8] = {0u};
	uint16_t crc_code = 0u;

	send_data[0] = 0x50;			//imu's id
	send_data[1] = 0x03;
	send_data[2] = 0x00;
	send_data[3] = 0x34;			//start register (send_data[2] << 8 | send_data[3])
	send_data[4] = 0x00;
	send_data[5] = 0x0C;			//request data size (send_data[4]<<8 | send_data[5])

	crc_code = getCrc(send_data, 6);

	send_data[6] = crc_code >> 8;		//crc code
	send_data[7] = crc_code & 0xff;		//crc code

	serial.writeSerial(send_data, 8);
}

void accelCalibration(void){
	uint8_t write_data[5] = {0u};

	write_data[0] = 0xff;
	write_data[1] = 0xaa;
	write_data[2] = 0x69;
	write_data[3] = 0x88;
	write_data[4] = 0x5b;

	serial.writeSerial(write_data, 5);		//unlock imu

	rclcpp::sleep_for(std::chrono::milliseconds(100));

	write_data[2] = 0x01;
	write_data[3] = 0x01;
	write_data[4] = 0x00;

	serial.writeSerial(write_data, 5);		//start calibration

	rclcpp::sleep_for(std::chrono::milliseconds(5500));
}

uint16_t getCrc(uint8_t *datas_, uint8_t size_){
	uint8_t crc_low = 0xff;
	uint8_t crc_high = 0xff;
	uint8_t crc_tmp = 0xff;

	//CRC array
	uint8_t crc_high_array[256] = { 
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40};
	uint8_t crc_low_array[256] = { 
		0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
		0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
		0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
		0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
		0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
		0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
		0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
		0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
		0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
		0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
		0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
		0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
		0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
		0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
		0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
		0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
		0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
		0x40};

	for(uint8_t i=0; i<size_;i++){
		crc_tmp  = (crc_high ^ datas_[i]) & 0xff;
		crc_high = (crc_low ^ crc_high_array[crc_tmp]) & 0xff;
		crc_low  = crc_low_array[crc_tmp];
	}

	return crc_high << 8 | crc_low;
}

//change output screen of serial_connect
void SerialConnect::errorSerial(std::string error_str_){
	if(error_out){
		RCLCPP_ERROR(node->get_logger(), "Serial Fail: %s %s", error_str_.c_str(), device_name.c_str());
	}
}

void SerialConnect::infoSerial(std::string info_str_){
	RCLCPP_INFO(node->get_logger(), "Serial Info: %s %s", info_str_.c_str(), device_name.c_str());
}