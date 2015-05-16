#ifndef TIME_FUN_H
#define TIME_FUN_H

#include <string>
#include <time.h>

time_t getTime_t(int y, int m, int d, int h, int n, int s);

// return "mm/dd/yyyy hh:nn:ss"
std::string printDateTime(time_t t);

// return "yyyy-mm-dd_hh:nn:ss"
std::string printDateTimeCsv(time_t t);

// return "yyymmdd_hhnnss"
std::string printDateTimeFileName();

#endif 
