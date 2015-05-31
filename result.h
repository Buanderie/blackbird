#ifndef RESULT_H
#define RESULT_H

#include <time.h>
#include <string>
#include <vector>

// structure that represents the result of a complete long/short trade (i.e. 2 trades IN, 2 trades OUT)
// some information is filled after the first two trades IN
struct Result {

  unsigned id;
  unsigned idExchLong;
  unsigned idExchShort;
  double exposure;
  double feesLong;
  double feesShort;
  time_t entryTime;
  time_t exitTime;
  std::string exchNameLong;
  std::string exchNameShort;
  double priceLongIn;
  double priceShortIn;
  double priceLongOut;
  double priceShortOut;
  double spreadIn;
  double spreadOut;

  double minSpread[8][8]; // FIXME size
  double maxSpread[8][8]; // FIXME size

  double befBalUsd;
  double aftBalUsd;
  
  // return the *target* performance
  // including exchange fees
  double perfLong();
  double perfShort();

 double totPerf();

  // get the length of the trade in minutes
  double getLength();
  
  // print to the console thr Entry/Exit trades information
  void printEntry();
  void printExit();
  
  // clears the structure
  // i.e. all the variables at 0
  void clear();

};

#endif
