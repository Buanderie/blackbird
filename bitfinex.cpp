#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "base64.h"
#include <jansson.h>
#include "bitfinex.h"
#include "curl_fun.h"

namespace Bitfinex {

double getQuote(CURL *curl, bool isBid) {

  json_t *root = getJsonFromUrl(curl, "https://api.bitfinex.com/v1/ticker/btcusd", "");
  if (isBid) {
    return atof(json_string_value(json_object_get(root, "bid")));
  } else {
    return atof(json_string_value(json_object_get(root, "ask")));
  }
}


double getAvail(CURL *curl, Parameters params, std::string currency) {

  json_t *root = authRequest(curl, params, "https://api.bitfinex.com/v1/balances", "balances", "");
  while (json_object_get(root, "message") != NULL) {
    std::cout << "<Bitfinex> Error with JSON: " << json_dumps(root, 0) << ". Retrying..." << std::endl;
    root = authRequest(curl, params, "https://api.bitfinex.com/v1/balances", "balances", "");
  }

  double availability = 0.0;

  // go through the list (order not preserved for some reason)
  size_t arraySize = json_array_size(root);

  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpType = json_string_value(json_object_get(json_array_get(root, i), "type"));
    std::string tmpCurrency = json_string_value(json_object_get(json_array_get(root, i), "currency"));
    if (tmpType.compare("trading") == 0 && tmpCurrency.compare(currency.c_str()) == 0) {
      availability = atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
    }
  }
  return availability;
}


int sendOrder(CURL *curl, Parameters params, std::string direction, double quantity, double price) {

  // define limit price to be sure to be executed
  double limPrice = 0;
  if (direction.compare("buy") == 0) {
    limPrice = getLimitPrice(curl, quantity, false);
  }
  else if (direction.compare("sell") == 0) {
    limPrice = getLimitPrice(curl, quantity, true);
  }

  std::cout << "<Bitfinex> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << limPrice << "..." << std::endl;
  std::ostringstream oss;
  oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity << "\", \"price\":\"" << limPrice << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction << "\", \"type\":\"limit\"";
  std::string options = oss.str();

  json_t *root = authRequest(curl, params, "https://api.bitfinex.com/v1/order/new", "order/new", options);
  int orderId = json_integer_value(json_object_get(root, "order_id"));
  std::cout << "<Bitfinex> Done (order ID: " << orderId << ")\n" << std::endl;

  return orderId;
}


bool isOrderComplete(CURL *curl, Parameters params, int orderId) {
  
  if (orderId == 0) {
    return true;
  }
  std::ostringstream oss;
  oss << "\"order_id\":" << orderId;
  std::string options = oss.str();

  json_t *root = authRequest(curl, params, "https://api.bitfinex.com/v1/order/status", "order/status", options);

  return !json_boolean_value(json_object_get(root, "is_live"));
}


double getActivePos(CURL *curl, Parameters params) {

  json_t *root = authRequest(curl, params, "https://api.bitfinex.com/v1/positions", "positions", "");

  if (json_array_size(root) == 0) {
    std::cout << "<Bitfinex> WARNING: BTC position not available, return 0.0" << std::endl;
    return 0.0;
  } else {  
    return atof(json_string_value(json_object_get(json_array_get(root, 0), "amount")));
  }
}


double getLimitPrice(CURL *curl, double volume, bool isBid) {

  json_t *root;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(curl, "https://api.bitfinex.com/v1/book/btcusd", ""), "bids"); 
  } else {
    root = json_object_get(getJsonFromUrl(curl, "https://api.bitfinex.com/v1/book/btcusd", ""), "asks"); 
  }
  // loop on volume
  double tmpVol = 0.0;
  int i = 0;
  while (tmpVol < volume) {
    // volumes are added up until the requested volume is reached
    tmpVol += atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
    i++;
  }
  // return the second next offer
  return atof(json_string_value(json_object_get(json_array_get(root, i+1), "price")));
}


json_t* authRequest(CURL *curl, Parameters params, std::string url, std::string request, std::string options) {

  std::string readBuffer;
  std::ostringstream oss;
 
  // nonce
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long nonce = (tv.tv_sec * 1000) + (tv.tv_usec / 1000.0) + 0.5;

  // check if options parameter is empty
  if (options.empty()) {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\"}";
  }
  else {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\", " << options << "}";
  }
  std::string tmpPayload = base64_encode(reinterpret_cast<const unsigned char*>(oss.str().c_str()), oss.str().length());

  oss.clear();
  oss.str("");

  oss << "X-BFX-PAYLOAD:" << tmpPayload;
  std::string payload;
  payload = oss.str();

  oss.clear();
  oss.str("");

  // build the signature
  unsigned char* digest;

  // Using sha384 hash engine
  digest = HMAC(EVP_sha384(), params.bitfinexSecret, strlen(params.bitfinexSecret), (unsigned char*)tmpPayload.c_str(), strlen(tmpPayload.c_str()), NULL, NULL);

  char mdString[SHA384_DIGEST_LENGTH+100];   // FIXME +100
  for (int i = 0; i < SHA384_DIGEST_LENGTH; ++i) {
    sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
  }
  oss.clear();
  oss.str("");
  oss << "X-BFX-SIGNATURE:" << mdString;

  // cURL headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, params.bitfinexApi);
  headers = curl_slist_append(headers, payload.c_str());
  headers = curl_slist_append(headers, oss.str().c_str());

  // cURL request
  CURLcode resCurl;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(curl);
    json_t *root;
    json_error_t error;

    while (resCurl != CURLE_OK) {
      std::cout << "<Bitfinex> Error with cURL. Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);

    while (!root) {
      std::cout << "<Bitfinex> Error with JSON:\n" << error.text << ". Retrying..." << std::endl;
      readBuffer = ""; 
      resCurl = curl_easy_perform(curl);
      while (resCurl != CURLE_OK) {
        std::cout << "<Bitfinex> Error with cURL. Retry in 2 sec...\n" << std::endl;
        sleep(2.0); 
        readBuffer = ""; 
        resCurl = curl_easy_perform(curl);
      } 
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_slist_free_all(headers);
    curl_easy_reset(curl);
    return root;
  }
  else {
    std::cout << "<Bitfinex> Error with cURL init." << std::endl;
    return NULL;
  }
}

}
