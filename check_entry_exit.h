#ifndef CHECK_ENTRY_EXIT_H
#define CHECK_ENTRY_EXIT_H

#include <time.h>
#include "bitcoin.h"
#include "parameters.h"
#include "result.h"

// check for long/short entry opportunity between two exchanges
bool checkEntry(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters params);

// check for long/short exit opportunity between two exchanges
bool checkExit(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters params, time_t period);

#endif 
