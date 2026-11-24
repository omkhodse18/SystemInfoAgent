#pragma once

#include "Command.h"

namespace SystemInfoAgent::cli
{
	class ICommandHandler
	{
	public:
		~ICommandHandler() = default;
		virtual int run(const Command& command) = 0;
	};

	class Screenshot : public ICommandHandler
	{

	};

}