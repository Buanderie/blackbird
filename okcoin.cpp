#include <string.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "base64.h"
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <jansson.h>
#include "okcoin.h"
#include "curl_fun.h"


double getOkCoinAvail(CURL* curl, Parameters params, std::string currency) {

  std::ostringstream oss;
  oss << "api_key=" << params.okCoinApi << "&secret_key=" << params.okCoinSecret;
  std::string signature(oss.str());
  oss.clear();
  oss.str("");
  oss << "api_key=" << params.okCoinApi;
  std::string content(oss.str());

  json_t *root = okCoinRequest(curl, "https://www.okcoin.com/api/v1/userinfo.do", signature, content);
  double available = 0.0;
  if (currency.compare("usd") == 0) {
    available = atof(json_string_value(json_object_get(json_object_get(json_object_get(json_object_get(root, "info"), "funds"), "free"), "usd"))); 
  }
  else if (currency.compare("btc") == 0) {
    available = atof(json_string_value(json_object_get(json_object_get(json_object_get(json_object_get(root, "info"), "funds"), "free"), "btc"))); 
  }
  return available;
}


int sendOkCoinOrder(CURL* curl, Parameters params, std::string direction, double quantity, double price) {

  std::ostringstream oss;
  std::string signature;
  std::string content;

  // define limit price to be sure to be executed
  double limPrice = 0;
  if (direction.compare("buy") == 0) {
    limPrice = getOkCoinLimitPrice(curl, quantity, false);
  }
  else if (direction.compare("sell") == 0) {
    limPrice = getOkCoinLimitPrice(curl, quantity, true);
  }

  // signature
  oss << "amount=" << quantity << "&api_key=" << params.okCoinApi << "&price=" << limPrice << "&symbol=btc_usd&type=" << direction << "&secret_key=" << params.okCoinSecret;
  signature = oss.str();
  oss.clear();
  oss.str("");
  // content
  oss << "amount=" << quantity << "&api_key=" << params.okCoinApi << "&price=" << limPrice << "&symbol=btc_usd&type=" << direction;
  content = oss.str();

  std::cout << "<OKCoin> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << limPrice << "..." << std::endl;
  json_t *root = okCoinRequest(curl, "https://www.okcoin.com/api/v1/trade.do", signature, content);
  int orderId = json_integer_value(json_object_get(root, "order_id")); 
  std::cout << "<OKCoin> Done (order ID: " << orderId << ")\n" << std::endl;

  return orderId;
}


bool isOkCoinOrderComplete(CURL* curl, Parameters params, int orderId) {
 
  int status = 0;
  
  std::ostringstream oss;
  std::string signature;
  std::string content;

  // signature
  oss << "api_key=" << params.okCoinApi << "&order_id=" << orderId << "&symbol=btc_usd" << "&secret_key=" << params.okCoinSecret;
  signature = oss.str();
  oss.clear();
  oss.str("");
  // content
  oss << "api_key=" << params.okCoinApi << "&order_id=" << orderId << "&symbol=btc_usd";
  content = oss.str();

  json_t *root = okCoinRequest(curl, "https://www.okcoin.com/api/v1/order_info.do", signature, content);
  
  status = json_integer_value(json_object_get(json_array_get(json_object_get(root, "orders"), 0), "status"));
  if (status == 2) {
    return true;
  } else {
    return false;
  }
}


double getActiveOkCoinPosition(CURL* curl, Parameters params) {
  return getOkCoinAvail(curl, params, "btc");
}


double getOkCoinLimitPrice(CURL* curl, double volume, bool isBid) {

  json_t *root;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(curl, "https://www.okcoin.com/api/v1/depth.do"), "bids");
    // loop on volume
    double tmpVol = 0;
    size_t i = 0;
    while (tmpVol < volume) {
      // volumes are added up until the requested volume is reached
      tmpVol += json_real_value(json_array_get(json_array_get(root, i), 1));
      i++;
    }
    // return the next offer
    return json_real_value(json_array_get(json_array_get(root, i), 0));

 } else {
    root = json_object_get(getJsonFromUrl(curl, "https://www.okcoin.com/api/v1/depth.do"), "asks");
    // loop on volume
    double tmpVol = 0;
    size_t i = json_array_size(root) - 1;
    while (tmpVol < volume) {
      // volumes are added up until the requested volume is reached
      tmpVol += json_real_value(json_array_get(json_array_get(root, i), 1));
      i--;
    }
    // return the next offer
    return json_real_value(json_array_get(json_array_get(root, i), 0));
  }
}


json_t* okCoinRequest(CURL* curl, std::string url, std::string signature, std::string content) {

  std::string readBuffer;

  // build the signature using MD5 hash engine
  unsigned char digest[MD5_DIGEST_LENGTH];

  MD5((unsigned char*)signature.c_str(), strlen(signature.c_str()), (unsigned char*)&digest);

  char mdString[33];
  for (int i = 0; i < 16; i++) {
    sprintf(&mdString[i*2], "%02X", (unsigned int)digest[i]);
  }

  std::ostringstream oss;
  oss << content << "&sign=" << mdString;

  std::string postParameters = oss.str().c_str();

  // cURL headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "contentType: application/x-www-form-urlencoded");

  // cURL request
  CURLcode resCurl;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_POST,1L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParameters.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(curl);
   
    while (resCurl != CURLE_OK) {
      std::cout << "Error with cURL! Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      resCurl = curl_easy_perform(curl);
    }

    // reset cURL
    curl_slist_free_all(headers);
    curl_easy_reset(curl);
    
    json_error_t error;
    return json_loads(readBuffer.c_str(), 0, &error);
  }
  else {
    std::cout << "ERROR" << std::endl;
    return NULL;
  }
}
