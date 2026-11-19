#pragma once
#include <vector>
#include "ProcessInfo.h"

namespace SystemInfoAgent::procinfo 
{
	class ProcessEnumerator {
	public:
		static std::vector<ProcessInfo> snapshot();
	};

}	// namespace SystemInfoAgent::procinfo