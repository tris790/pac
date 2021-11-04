//#include "config.h"
#include <iostream>
#include <string>
#include <map>
#include <fstream>

class Config
{
public:
	Config(std::string filename);
	~Config();
	bool readFile();
	std::map <std::string, std::string> conf_dict;
private:
	std::ifstream ifp;
};