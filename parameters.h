#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <time.h>
#include <string>
#include <vector>
#include <jansson.h>

// this structure contains the exchanges and
// the strategy parameters
struct Parameters {
  
  // exchanges information
  std::vector<std::string> exchName;
  std::vector<double> fees;
  std::vector<bool> hasShort;
  std::vector<std::string> tickerUrl;
    
  // strategy parameters
  double spreadEntry;
  double spreadExit;
  unsigned maxLength;
  
  // verbose or non-verbose mode
  bool verbose;
    
  // credentials
  const char *bitfinexApi;
  const char *bitfinexSecret;
  const char *okCoinApi;
  const char *okCoinSecret;
  const char *bitstampClientId;
  const char *bitstampApi;
  const char *bitstampSecret;

  // email
  bool sendEmail;
  const char *senderAddress;
  const char *senderUsername;
  const char *senderPassword;
  const char *smtpServerAddress;
  const char *receiverAddress;

  // constructor
  Parameters(json_t *root);

  // adds a new exchange
  void addExchange(std::string n, double f, bool h);

  // returns the number of exchange analyzed
  unsigned nbExch() const;  
  
};
  
#endif
