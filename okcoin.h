#ifndef OKCOIN_H
#define OKCOIN_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

// get the current availility for usd or btc
double getOkCoinAvail(CURL* curl, Parameters params, std::string currency);

// send order to the exchange
// return order ID
int sendOkCoinOrder(CURL* curl, Parameters params, std::string direction, double quantity, double price);

// check the status of the order
bool isOkCoinOrderComplete(CURL* curl, Parameters params, int orderId);

// get the bitcoin exposition
double getActiveOkCoinPosition(CURL* curl, Parameters params);

// get the limit price according to the requested volume
double getOkCoinLimitPrice(CURL* curl, double volume, bool isBid);

// send a request to the exchange and return a json object
json_t* okCoinRequest(CURL* curl, std::string url, std::string signature, std::string content);

#endif
