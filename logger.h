#ifndef LOGGER
#define LOGGER

#include <sys/time.h>
#include <iostream>
#include <stdio.h>

class Logger {
public:

	template
	<typename T>
	Logger& operator<< (const T& variable) {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		int ms = (tp.tv_usec / 1000 % 1000);
		char milisec[10];
		sprintf(milisec, "%03d", ms);

		time_t now = time(0);
		struct tm  tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%Y/%m/%d %X", &tstruct);

		std::cout << buf << "." << milisec << "    " << variable << std::endl;
		return *this;
	}
};

extern Logger logger;

#endif


