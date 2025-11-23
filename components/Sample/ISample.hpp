#pragma once
#include <cstdint>

#define USE_TIMESTAMP 1

struct ISample {
	#if USE_TIMESTAMP
	uint64_t timestamp;
	
	#endif
};