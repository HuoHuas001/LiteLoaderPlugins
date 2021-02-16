#include "pch.h"
#include "Logger.h"
#define endl "\n";
#pragma warning(disable:4996)

inline string gettime()
{
	time_t rawtime;
	tm* LocTime;
	char timestr[20];
	time(&rawtime);
	LocTime = localtime(&rawtime);
	strftime(timestr, 20, "%Y-%m-%d %H:%M:%S", LocTime);
	return string(timestr);
}

class Logger {
	std::string path;
	std::ofstream logfile;
public:
	Logger(std::string path) {
		this->path = path;
		logfile.open(path, std::ios::app);
		logfile << "[" << gettime() << " INFO ] Server Started" << endl;
	}
	template <typename T>
	Logger& operator<< (T msg) {
		logfile << msg;
		std::cout << msg;
		return *this;
	}
};