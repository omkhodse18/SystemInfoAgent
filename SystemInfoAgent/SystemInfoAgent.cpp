#include <iostream>

#include "Command.h"
#include "commandParser.h"

using namespace SystemInfoAgent::cli;


int wmain(int argc, wchar_t* argv[])
{
	Command command = CommandParser::parse(argc, argv);

}