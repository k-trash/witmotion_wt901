#ifndef __WITMOTION_LIB__
#define __WITMOTION_LIB__

#include <vector>
#include <termios.h>

typedef struct Boudrate{
	uint32_t int_boud;
	speed_t speed_boud;
}Boudrate;

const std::vector<Boudrate> boudrates = {{9600u, B9600}, {19200u, B19200}, {38400u, B38400}, {57600u, B57600}, {115200u, B115200}, {230400u, B230400}};

speed_t checkBoudrate(std::string boudrate_);

#endif