#ifndef KRAKEN_H
#define KRAKEN_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

namespace Kraken {

  // get quote
  double getQuote(CURL *curl, bool isBid);

  // get the current availability for usd or btc
  double getAvail(CURL *curl, Parameters params, std::string currency);

  // get the bitcoin exposition
  double getActivePos(CURL *curl, Parameters params);

}
#endif
