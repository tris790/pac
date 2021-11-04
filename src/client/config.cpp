#include "config.h"
#include <string>

Config::Config(std::string filename )
{
	//Config::fp = NULL;
	//Config::fp = fopen(filename.c_str(), "rb+");
	Config::ifp.open(filename.c_str());

	if (!Config::readFile())
	{
		std::cout << "Probl�me � lire le fichier de configuration" << std::endl;
		Config::ifp.close();
		exit(1);
	}
}

Config::~Config()
{
	if (Config::ifp.is_open())
	{
		Config::ifp.close();
	}
}

bool Config::readFile()
{
	std::string line;
	std::string delim = "=";
	if (Config::ifp.is_open())
	{
		try
		{
			while (std::getline(Config::ifp, line))
			{
				size_t pos = line.find(delim);
				Config::conf_dict.emplace(
					line.substr(0, pos),
					line.substr(pos+delim.length(), line.length())
				);
			}
		}
		catch (const std::exception&)
		{
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}