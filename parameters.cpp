#include <iostream>
#include <iomanip>
#include "parameters.h"
#include "time_fun.h"

// constructor
Parameters::Parameters(json_t *root) {
    
  // strategy and global parameters  
  spreadEntry = json_real_value(json_object_get(root, "SpreadEntry"));
  spreadExit = json_real_value(json_object_get(root, "SpreadExit"));
  maxLength = json_integer_value(json_object_get(root, "MaxLength"));
  verbose = json_boolean_value(json_object_get(root, "Verbose"));

  // exchanges credentials
  bitfinexApi = json_string_value(json_object_get(root, "BitfinexApiHead"));
  bitfinexSecret = json_string_value(json_object_get(root, "BitfinexKey"));
  okCoinApi = json_string_value(json_object_get(root, "OkCoinApiKey"));
  okCoinSecret = json_string_value(json_object_get(root, "OkCoinSecretKey"));
  bitstampClientId = json_string_value(json_object_get(root, "BitstampClientId"));
  bitstampApi = json_string_value(json_object_get(root, "BitstampApiKey"));
  bitstampSecret = json_string_value(json_object_get(root, "BitstampSecretKey"));

  // email
  sendEmail = json_boolean_value(json_object_get(root, "SendEmail"));
  senderAddress = json_string_value(json_object_get(root, "SenderAddress"));
  senderUsername = json_string_value(json_object_get(root, "SenderUsername"));
  senderPassword = json_string_value(json_object_get(root, "SenderPassword"));
  smtpServerAddress = json_string_value(json_object_get(root, "SmtpServerAddress"));
  receiverAddress = json_string_value(json_object_get(root, "ReceiverAddress"));
}


// add an exchange
void Parameters::addExchange(std::string n, double f, bool h) {
  exchName.push_back(n);
  fees.push_back(f);
  hasShort.push_back(h);
}


// returns number of exchanges
unsigned Parameters::nbExch() const {
  return exchName.size();
}

