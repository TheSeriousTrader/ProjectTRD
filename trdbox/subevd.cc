//
// subevent_builder_app.cpp
//
// This sample demonstrates the ServerApplication class.
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//

#include "trd_subevent_builder.hh"

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/MapConfiguration.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
// #include "Poco/DateTimeFormatter.h"
#include "Poco/Format.h"
#include <iostream>
#include <zmq.hpp>


using Poco::Util::Application;
using Poco::Util::ServerApplication;
using Poco::Util::MapConfiguration;
using Poco::Util::AbstractConfiguration;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::format;
// using Poco::Task;
using Poco::TaskManager;
// using Poco::DateTimeFormatter;

// class SampleTask: public Task
// {
// public:
// 	SampleTask(): Task("SampleTask")
// 	{
// 	}
//
// 	void runTask()
// 	{
// 		Application& app = Application::instance();
// 		while (!sleep(5000))
// 		{
// 			Application::instance().logger().information("busy doing nothing... " + DateTimeFormatter::format(app.uptime()));
// 		}
// 	}
// };
//

class subevent_builder_app: public ServerApplication
{
public:
	subevent_builder_app(): _helpRequested(false), zmqcontext(1)
	{
	}

	~subevent_builder_app()
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
		// logger().information("starting up");
	}

	void uninitialize()
	{
		// logger().information("shutting down");
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options)
	{
		ServerApplication::defineOptions(options);

		options.addOption(
			Option("sfp0-enable", "", "enable event builder for sfp0")
			.required(false)
			.argument("bool")
			.binding("sfp0.enable"));

		options.addOption(
			Option("sfp1-enable", "", "enable event builder for sfp1")
			.required(false)
			.argument("bool")
			.binding("sfp1.enable"));

		options.addOption(
			Option("timeout", "", "timeout for reading from TRDbox buffer (in microseconds)")
			.required(false)
			.argument("int")
			.binding("timeout"));

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<subevent_builder_app>(this, &subevent_builder_app::handleHelp)));
}

void handleHelp(const std::string& name, const std::string& value)
{
	_helpRequested = true;
	displayHelp();
	stopOptionsProcessing();
}

void displayHelp()
{
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("TRD Subevent Builder Daemon");
	helpFormatter.format(std::cout);

	// std::cout << config().getBool("sfp0.enable") << std::endl;
}


int main(const ArgVec& args)
{
	if (!_helpRequested)
	{
		TaskManager tm;

		for (int s=0; s<2; s++) {
			std::string key = format("sfp%d.enable",s);
			if ( config().has(key) && config().getBool(key) ) {
				auto cfg = config().createView(format("sfp%d",s));
				tm.start(new trd_subevent_builder(zmqcontext, s, cfg));
			}
		}

		waitForTerminationRequest();
		tm.cancelAll();
		tm.joinAll();
	}
	return Application::EXIT_OK;
}

void printProperties(const std::string& base)
	{
		AbstractConfiguration::Keys keys;
		config().keys(base, keys);
		if (keys.empty())
		{
			if (config().hasProperty(base))
			{
				std::string msg;
				msg.append(base);
				msg.append(" = ");
				msg.append(config().getString(base));
				logger().information(msg);
			}
		}
		else
		{
			for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				std::string fullKey = base;
				if (!fullKey.empty()) fullKey += '.';
				fullKey.append(*it);
				printProperties(fullKey);
			}
		}
	}

private:
	bool _helpRequested;
	zmq::context_t zmqcontext;
};


POCO_SERVER_MAIN(subevent_builder_app)
