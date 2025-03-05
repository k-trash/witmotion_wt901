#include <witmotion_wt901/witmotion.hpp>

#include <string>
#include <cstdlib>
#include <termios.h>

speed_t checkBaudrate(const std::string baudrate_){
	std::string baud_str;
	if(baudrate_[0] == 'B'){
		baud_str = baudrate_.substr(1);
	}else{
		baud_str = baudrate_;
	}

	for(auto itr=baudrates.begin(); itr!=baudrates.end();itr++){
		if(itr->int_baud == std::atoi(baud_str.c_str())){
			return itr->speed_baud;
		}
	}

	return 0;
}
