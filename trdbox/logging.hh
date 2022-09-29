#ifndef TRDBOX_LOGGING_HH
#define TRDBOX_LOGGING_HH

#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/LogStream.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/SyslogChannel.h>

extern Poco::Logger& logger;
extern Poco::LogStream logs;

#endif
