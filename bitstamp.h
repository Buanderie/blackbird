#ifndef BITSTAMP_H
#define BITSTAMP_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

namespace Bitstamp {

// get quote
double getQuote(CURL* curl, bool isBid);

// get the current availability for usd and btc
double getAvail(CURL *curl, Parameters params, std::string currency);

// send order to the exchange and return order ID
int sendOrder(CURL *curl, Parameters params, std::string direction, double quantity, double price);

// check the status of the order
bool isOrderComplete(CURL *curl, Parameters params, int orderId);

// get the Bitcoin exposition
double getActivePos(CURL *curl, Parameters params);

// get the limit price according to the requested volume
double getLimitPrice(CURL *curl, double volume, bool isBid);

// send a request to the exchange and return a JSON object
json_t* authRequest(CURL *curl, Parameters params, std::string url, std::string options);

}

#endif
