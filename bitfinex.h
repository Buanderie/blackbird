#ifndef BITFINEX_H
#define BITFINEX_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

// get the current availability for usd or btc
double getBitfinexAvail(CURL* curl, Parameters params, std::string currency);

// send order to the exchange
// return order ID
int sendBitfinexOrder(CURL* curl, Parameters params, std::string direction, double quantity, double price);

// check the status of the order
bool isBitfinexOrderComplete(CURL* curl, Parameters params, int orderId);

// get the bitcoin exposition
double getActiveBitfinexPosition(CURL* curl, Parameters params);

// get the limit price according to the requested volume
double getBitfinexLimitPrice(CURL* curl, double volume, bool isBid);

// send a request to the exchange and return a json object
json_t* bitfinexRequest(CURL* curl, Parameters params, std::string url, std::string request, std::string options);

#endif
