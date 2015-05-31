#include <string.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <jansson.h>
#include "kraken.h"
#include "curl_fun.h"

namespace Kraken {

  double getQuote(CURL *curl, bool isBid) {
    
    json_t *root = getJsonFromUrl(curl, "https://api.kraken.com/0/public/Ticker", "pair=XXBTZUSD");
    if (isBid) {
      return atof(json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "b"), 0)));
    } else {
      return atof(json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "a"), 0))); 
    }
  }


  double getAvail(CURL *curl, Parameters params, std::string currency) {
    return 0.0;
  }

  
  double getActivePos(CURL *curl, Parameters params) {
    return 0.0;
  }

}
