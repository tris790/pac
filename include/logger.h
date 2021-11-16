#pragma once
#include <chrono>
#include <fstream>
#include <iomanip>

// Time format
#define LOGGER_PRETTY_TIME_FORMAT "%Y-%m-%d %H:%M:%S"

class Logger
{
public:
    Logger() {}

    void fatal(const std::string &message) { fatal(std::cout, message); }

    void fatal(std::ostream &outputFile, const std::string &message) { log("FATAL", message, outputFile); }

    template <typename... Args>
    void fatal(const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        fatal(messageFormated);
    }

    template <typename... Args>
    void fatal(std::ostream &outputFile, const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        fatal(outputFile, messageFormated);
    }

    void error(const std::string &message) { error(std::cout, message); }

    void error(std::ostream &outputFile, const std::string &message) { log("ERROR", message, outputFile); }

    template <typename... Args>
    void error(const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        error(messageFormated);
    }

    template <typename... Args>
    void error(std::ostream &outputFile, const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        error(outputFile, messageFormated);
    }

    void warning(const std::string &message) { warning(std::cout, message); }

    void warning(std::ostream &outputFile, const std::string &message) { log("WARN", message, outputFile); }

    template <typename... Args>
    void warning(const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        warning(messageFormated);
    }

    template <typename... Args>
    void warning(std::ostream &outputFile, const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        warning(outputFile, messageFormated);
    }

    void info(const std::string &message) { info(std::cout, message); }

    void info(std::ostream &outputFile, const std::string &message) { log("INFO", message, outputFile); }

    template <typename... Args>
    void info(const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        info(messageFormated);
    }

    template <typename... Args>
    void info(std::ostream &outputFile, const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        info(outputFile, messageFormated);
    }

    void debug(const std::string &message) { debug(std::cout, message); }

    void debug(std::ostream &outputFile, const std::string &message) { log("DEBUG", message, outputFile); }

    template <typename... Args>
    void debug(const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        debug(messageFormated);
    }

    template <typename... Args>
    void debug(std::ostream &outputFile, const std::string &message, Args... args)
    {
        auto messageFormated = stringFormat(message, args...);
        debug(outputFile, messageFormated);
    }

private:
    void log(const std::string &level, const std::string &message, std::ostream &outputFile = std::cout)
    {
        std::stringstream output;

        auto time = getCurrentTime();

        // Add time
        output << std::put_time(std::localtime(&time), LOGGER_PRETTY_TIME_FORMAT) << " ";

        // Add message
        output << level << " - " << message << std::endl;

        outputFile << output.str();
    }

    time_t getCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::system_clock::to_time_t(now);
    }

    // Source : https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    template <typename... Args>
    std::string stringFormat(const std::string &format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'

        if (size_s <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }

        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<char[]>(size);

        std::snprintf(buf.get(), size, format.c_str(), args...);

        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }
};

static Logger logger;