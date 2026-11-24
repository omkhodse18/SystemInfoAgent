#include <iostream>

#include "Command.h"
#include "commandParser.h"
#include "CommandHandlerInterface.h"

using namespace SystemInfoAgent::cli;

std::unique_ptr<ICommandHandler> makeHandler(Mode mode)
{
	switch (mode)
	{
	case Mode::Screenshot:
		return std::make_unique<ScreenshotHandler>();
	}
}

int dispatch(const Command& command)
{
	auto handler = makeHandler(command.mode);

	if (!handler) {
		return 1;
	}

	return handler->run(command);
}

int wmain(int argc, wchar_t* argv[])
{
	Command command = CommandParser::parse(argc, argv);
	return dispatch(command);
}