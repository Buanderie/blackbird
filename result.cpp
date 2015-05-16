#include <iostream>
#include <numeric>
#include "result.h"
#include "time_fun.h"

double Result::perfLong() {
  return (priceLongOut - priceLongIn) / priceLongIn - 2 * feesLong;
}


double Result::perfShort() {
  return (priceShortIn - priceShortOut) / priceShortIn - 2 * feesShort;
}


double Result::totPerf() {
  if (exposure == 0.0) {
    return 0.0;
  } else {
    return (aftBalUsd - befBalUsd) / (exposure * 2.0);
  }
}


double Result::getLength() {
  if (entryTime > 0 && exitTime > 0) {
    return ((double)(exitTime - entryTime)) / 60;
  }
  else {
    return 0;
  }
}


void Result::printEntry() {
  
  std::cout << "\n[ ENTRY FOUND ]" << std::endl;
  std::cout << "   Date & Time:       "  << printDateTime(entryTime) << std::endl;
  std::cout << "   Exchange Long:     "  << exchNameLong <<  " (id " << idExchLong  << ")" << std::endl;
  std::cout << "   Exchange Short:    "  << exchNameShort << " (id " << idExchShort << ")" << std::endl;
  std::cout << "   Fees:              "  << feesLong * 100.0 << "% / " << feesShort * 100.0 << "%" << std::endl;
  std::cout << "   Price Long:        $" << priceLongIn << " (target)" << std::endl;
  std::cout << "   Price Short:       $" << priceShortIn << " (target)" << std::endl;
  std::cout << "   Spread:            "  << spreadIn * 100.0 << "%" << std::endl;   
  std::cout << "   Cash used:         $" << exposure << " on each exchange" << std::endl;
  std::cout << std::endl;
}


void Result::printExit() {
  
  std::cout << "\n[ EXIT FOUND ]" << std::endl;
  std::cout << "   Date & Time:       "  << printDateTime(exitTime) << std::endl;
  std::cout << "   Duration:          "  << getLength() << " minutes" << std::endl;
  std::cout << "   Price Long:        $" << priceLongOut << " (target)" << std::endl;
  std::cout << "   Price Short:       $" << priceShortOut << " (target)" << std::endl;
  std::cout << "   Spread:            "  << spreadOut * 100.0 << "%" << std::endl;
  std::cout << "   ---------------------------"  << std::endl;
  std::cout << "   Target Perf Long:  "  << perfLong()  * 100.0 << "% (fees incl.)" << std::endl;
  std::cout << "   Target Perf Short: "  << perfShort() * 100.0 << "% (fees incl.)" << std::endl;
  std::cout << "   ---------------------------\n"  << std::endl;
}



void Result::clear() {  
  id = 0;
  idExchLong = 0;
  idExchShort = 0;
  exposure = 0.0;
  feesLong = 0.0;
  feesShort = 0.0;
  entryTime = 0;
  exitTime = 0;
  exchNameLong = "";
  exchNameShort = "";
  priceLongIn = 0.0;
  priceShortIn = 0.0;
  priceLongOut = 0.0;
  priceShortOut = 0.0;
  spreadIn = 0.0;
  spreadOut = 0.0;
  maxCurrSpread = -1.0;
  minCurrSpread = 1.0;
  befBalUsd = 0.0;
  aftBalUsd = 0.0;
}
