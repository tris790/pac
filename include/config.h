#pragma once
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include "logger.h"

class Config
{
public:
	Config(std::string filename)
	{
		// std::fstream::in (read-only access)
		Config::ifp.open(filename.c_str(), std::fstream::in);

		bool file_was_read = Config::readFile();
		Config::ifp.close();
		if (!file_was_read)
		{
			std::cout << "Can't read the config file: " << filename << std::endl;
			exit(1);
		}
	}

	// So we can access the config like this Config[key]
	std::string &operator[](std::string key)
	{
		return conf_dict[key];
	}

	bool readFile()
	{
		Config::conf_dict.clear();
		std::string current_line;
		std::string delim = "=";
		if (Config::ifp.is_open())
		{
			try
			{
				while (std::getline(Config::ifp, current_line))
				{
					size_t pos = current_line.find(delim);
					auto key = current_line.substr(0, pos);
					auto value = current_line.substr(pos + delim.length(), current_line.length());

					value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
					value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
				}
			}
			catch (const std::exception &e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

private:
	std::ifstream ifp;
	std::map<std::string, std::string> conf_dict;
};