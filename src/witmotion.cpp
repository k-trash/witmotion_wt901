#include <witmotion_wt901/witmotion.hpp>

#include <string>
#include <cstdlib>
#include <termios.h>

speed_t checkBoudrate(const std::string boudrate_){
	std::string boud_str;
	if(boudrate_[0] == 'B'){
		boud_str = boudrate_.substr(1);
	}else{
		boud_str = boudrate_;
	}

	for(auto itr=baudrates.begin(); itr!=boudrates.end();itr++){
		if(itr->int_boud == std::atoi(boud_str.c_str())){
			return itr->speed_boud;
		}
	}

	return 0;
}
