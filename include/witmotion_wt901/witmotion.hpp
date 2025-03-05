#ifndef __WITMOTION_LIB__
#define __WITMOTION_LIB__

#include <vector>
#include <string>
#include <termios.h>

typedef struct Baudrate{
	uint32_t int_baud;
	speed_t speed_baud;
}Baudrate;

const std::vector<Baudrate> baudrates = {{9600u, B9600}, {19200u, B19200}, {38400u, B38400}, {57600u, B57600}, {115200u, B115200}, {230400u, B230400}};

speed_t checkBaudrate(const std::string baudrate_);

#endif