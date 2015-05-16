#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <numeric>
#include <math.h>
#include <algorithm>
#include <jansson.h>
#include <curl/curl.h>

#include "base64.h"
#include "bitcoin.h"
#include "result.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "parameters.h"
#include "check_entry_exit.h"
#include "bitfinex.h"
#include "okcoin.h"
#include "send_email.h"

int main(int argc, char **argv) {

  // header information
  std::cout << "Blackbird Bitcoin Arbitrage\nVersion 0.0.1, May 2015" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK.\n" << std::endl;

  // read the config file (config.json)
  json_error_t error;
  json_t *root = json_load_file("config.json", 0, &error);
  if (!root) {
     std::cout << "Error with the config file: " << error.text << ".\n" << std::endl;
    return 1;
  }
 
  // get config variables
  int gapSec = json_integer_value(json_object_get(root, "GapSec"));
  unsigned  debugMaxIteration = json_integer_value(json_object_get(root, "DebugMaxIteration"));

  bool useFullCash = json_boolean_value(json_object_get(root, "UseFullCash")); 
  double untouchedCash = json_real_value(json_object_get(root, "UntouchedCash"));
  double cashForTesting = json_real_value(json_object_get(root, "CashForTesting"));

  // thousand separator
  std::locale mylocale("");
  std::cout.imbue(mylocale);

  // print precision of two digits
  std::cout.precision(2);
  std::cout << std::fixed;

  // create a parameters structure
  Parameters params(root);
  params.addExchange("Bitfinex", 0.0020, true, "https://api.bitfinex.com/v1/ticker/btcusd");
  params.addExchange("OKCoin", 0.0020, false, "https://www.okcoin.com/api/ticker.do?ok=1");

  // CSV file
  std::string csvFileName;
  csvFileName = "result_" + printDateTimeFileName() + ".csv";
  std::ofstream csvFile;
  csvFile.open(csvFileName.c_str(), std::ofstream::trunc);
  // CSV header
  csvFile << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN\n";
  csvFile.flush();

  // time structure
  time_t rawtime;
  rawtime = time(NULL);
  struct tm * timeinfo;
  timeinfo = localtime(&rawtime);
  
  bool inMarket = false;
  int resultId = 0;
  Result res;
  res.clear();
  // Vector of Bitcoin
  std::vector<Bitcoin*> btcVec;
  // create Bitcoin objects:
  // 0. Bitfinex, 1. OKCoin
  Bitcoin *a = new Bitcoin(0, params.exchName[0], params.fees[0], params.hasShort[0]);
  btcVec.push_back(a);
  Bitcoin *b = new Bitcoin(1, params.exchName[1], params.fees[1], params.hasShort[1]);
  btcVec.push_back(b);
  
  // cURL
  CURL* curl;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  std::cout << "[ Targets ]" << std::endl;
  std::cout << "   Spread to enter: " << params.spreadEntry * 100.0 << "%" << std::endl;
  std::cout << "   Spread to exit:  " << params.spreadExit * 100.0  << "%" << std::endl;
  std::cout << std::endl;

  // store current balances
  std::cout << "[ Current balances ]" << std::endl;
  std::vector<double> balanceUsd;
  std::vector<double> balanceBtc;
 
  balanceUsd.push_back(getBitfinexAvail(curl, params, "usd"));
  balanceUsd.push_back(getOkCoinAvail(curl, params, "usd"));
  balanceBtc.push_back(getBitfinexAvail(curl, params, "btc"));
  balanceBtc.push_back(getOkCoinAvail(curl, params, "btc"));
  
  // vectors that contains balances after a completed trade
  std::vector<double> newBalUsd(params.nbExch(), 0.0);
  std::vector<double> newBalBtc(params.nbExch(), 0.0);
  
  for (unsigned i = 0; i < params.nbExch(); ++i) {
    std::cout << "   " << params.exchName[i] << ":\t";
    std::cout << balanceUsd[i] << " USD\t" << std::setprecision(6) << balanceBtc[i]  << std::setprecision(2) << " BTC" << std::endl;
  }
  std::cout << std::endl;

  std::cout << "[ Cash exposure ]" << std::endl;
  if (useFullCash) {
    double initCash = *std::min_element(balanceUsd.begin(), balanceUsd.end());
    initCash -= initCash * untouchedCash;
    std::cout << "   FULL cash used!\n   Initial value: $" << initCash << std::endl;
  } else {
    std::cout << "   TEST cash used\n   Value: $" << cashForTesting << std::endl;
  }
  std::cout << std::endl;

  // wait the next gapSec seconds before starting
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  while ((int)timeinfo->tm_sec % gapSec != 0) {
    sleep(0.01);
    time(&rawtime);
    timeinfo = localtime(&rawtime);
  }

  // cut to debug
  // std::cout << "Debug: exit before main loop.\n" << std::endl;
  // return 0;

  // MAIN LOOP
  if (!params.verbose) {
    std::cout << "Running..." << std::endl;  
  }

  unsigned currIteration = 0;
  while (currIteration < debugMaxIteration) {

    time_t currTime = mktime(timeinfo);
    time(&rawtime);

    // check if we are already too late
    if (difftime(rawtime, currTime) > 0) {
      std::cout << "WARNING: " << difftime(rawtime, currTime) << " second(s) too late at " << printDateTime(currTime) << std::endl;
      unsigned skip = (unsigned)ceil(difftime(rawtime, currTime) / gapSec);
      for (unsigned i = 0; i < skip; ++i) {
        // std::cout << "         Skipped iteration " << printDateTime(currTime) << std::endl;
        for (unsigned e = 0; e < params.nbExch(); ++e) {
          btcVec[e]->addData(currTime, btcVec[e]->getLastBid(), btcVec[e]->getLastAsk(), 0.0);
        }
        // go to next iteration
        timeinfo->tm_sec = timeinfo->tm_sec + gapSec;
        currTime = mktime(timeinfo);
      }
      std::cout << std::endl;
    }
    // wait for the next iteration
    while (difftime(rawtime, currTime) != 0) {
      sleep(0.01);
      time(&rawtime);
    }
    if (params.verbose) {
      if (!inMarket) {
        std::cout << "[ " << printDateTime(currTime) << " ]" << std::endl;
      }
      else {
        std::cout << "[ " << printDateTime(currTime) << " IN MARKET: Long " << res.exchNameLong << " / Short " << res.exchNameShort << " ]" << std::endl;
      }
    }
    // download the exchanges prices
    for (unsigned e = 0; e < params.nbExch(); ++e) {

      double bid = 0.0;
      double ask = 0.0;
      double lastTrade = 0.0;

      json_t *root = getJsonFromUrl(curl, params.tickerUrl[e]);
   
      if (!root) {
        if (params.verbose) {
          std::cout << "WARNING: Data error on " << params.exchName[e] << std::endl;
          std::cout << "         Add previous price (" << btcVec[e]->getLastBid() << " / " << btcVec[e]->getLastAsk() << ")" << std::endl;
        }
        btcVec[e]->addData(currTime, btcVec[e]->getLastBid(), btcVec[e]->getLastAsk(), 0.0);
      }
      else {
        // Bitfinex
        if (e == 0) {
          bid = atof(json_string_value(json_object_get(root, "bid")));
          ask = atof(json_string_value(json_object_get(root, "ask")));
          lastTrade = atof(json_string_value(json_object_get(root, "last_price")));           
        }
        // OKCoin
        else if  (e == 1) {
          bid = atof(json_string_value(json_object_get(json_object_get(root, "ticker"), "buy")));
          ask = atof(json_string_value(json_object_get(json_object_get(root, "ticker"), "sell")));
          lastTrade = atof(json_string_value(json_object_get(json_object_get(root, "ticker"), "last")));         
        }
        if (params.verbose) {
          std::cout << "   " << params.exchName[e] << ": \t" << bid << " / " << ask << "    Last trade: " << lastTrade << std::endl;
        }
        btcVec[e]->addData(currTime, bid, ask, 0.0);
      }
      curl_easy_reset(curl);
      json_decref(root);
    }
    // load data terminated
    if (params.verbose) {
      std::cout << "   --------------------------------------------------" << std::endl;
    }
    // compute entry point
    if (!inMarket) {
      for (unsigned i = 0; i < params.nbExch(); ++i) {
        for (unsigned j = 0; j < params.nbExch(); ++j) {
          if (i != j) {
            if (checkEntry(btcVec[i], btcVec[j], res, params)) {
              // entry opportunity found!
              // compute exposure
              res.exposure = std::min(balanceUsd[res.idExchLong], balanceUsd[res.idExchShort]);
	      if (res.exposure == 0.0) {
	        std::cout << "\nWARNING: Opportunity found but no cash available. Trade canceled." << std::endl;
		break;
              }
	      if (useFullCash == false && res.exposure <= cashForTesting) {
	        std::cout << "\nWARNING: Opportunity found but no enough cash. Need more than TEST cash (min. $" << cashForTesting << "). Trade canceled." << std::endl;
	        break;	
              }
              if (useFullCash) {
                // leave untouchedCash
                res.exposure -= untouchedCash * res.exposure;
              }
              else {
                // use test money
                res.exposure = cashForTesting;
	      }

              inMarket = true;
              resultId++;
              // update result
              res.id = resultId;
              res.entryTime = currTime;
              res.printEntry();
              res.maxCurrSpread = -1.0;
              res.minCurrSpread = 1.0;
              int bitfinexOrderId = 0;
              int okCoinOrderId = 0;

              // send orders
              if (res.idExchLong == 0) {
                // Long Bitfinex
                bitfinexOrderId = sendBitfinexOrder(curl, params, "buy", res.exposure / btcVec[res.idExchLong]->getLastAsk(), btcVec[res.idExchLong]->getLastAsk());
              }
              else if (res.idExchLong == 1) {
                // Long OkCoin
                okCoinOrderId = sendOkCoinOrder(curl, params, "buy", res.exposure / btcVec[res.idExchLong]->getLastAsk(), btcVec[res.idExchLong]->getLastAsk());
              }
              
              if (res.idExchShort == 0) {
                // Short Bitfinex
                bitfinexOrderId = sendBitfinexOrder(curl, params, "sell", res.exposure / btcVec[res.idExchShort]->getLastBid(), btcVec[res.idExchShort]->getLastBid());
              }
              else if (res.idExchShort == 1) {
                // Short OkCoin
                // To be implemented
              }
              
              // wait for the orders to be filled
              sleep(3.0);
              std::cout << "Waiting for the two orders to be filled..." << std::endl;
              while (!isBitfinexOrderComplete(curl, params, bitfinexOrderId) || !isOkCoinOrderComplete(curl, params, okCoinOrderId)) {
                sleep(3.0);
              }
              std::cout << "Done" << std::endl;
	      bitfinexOrderId = 0;
	      okCoinOrderId = 0;
              break;
            }
          }
        }
        if (inMarket) {
          break;
        }
      }
      if (params.verbose) {
        std::cout << std::endl;
      }
    }
    // in market, looking to exit
    else if (inMarket) {

      if (checkExit(btcVec[res.idExchLong], btcVec[res.idExchShort], res, params, currTime)) {
        // exit opportunity found!
        res.exitTime = currTime;
        res.printExit();
        // send orders
        // check current exposure
        std::vector<double> btcUsed;
        btcUsed.push_back(getActiveBitfinexPosition(curl, params));
        btcUsed.push_back(getActiveOkCoinPosition(curl, params));
        int bitfinexOrderId = 0;
	int okCoinOrderId = 0;
        
	for (unsigned i = 0; i < params.nbExch(); ++i) {
          std::cout << std::setprecision(6) << "BTC exposure on " << params.exchName[i] << ": " << btcUsed[i] << std::setprecision(2) << std::endl;
        }
        std::cout << std::endl;
        
        if (res.idExchLong == 0) {
          // Close Long Bitfinex
          bitfinexOrderId = sendBitfinexOrder(curl, params, "sell", fabs(btcUsed[res.idExchLong]), btcVec[res.idExchLong]->getLastBid());
        }
        else if (res.idExchLong == 1) {
          // Close Long OkCoin
          okCoinOrderId = sendOkCoinOrder(curl, params, "sell", fabs(btcUsed[res.idExchLong]), btcVec[res.idExchLong]->getLastBid());
        }
        
        if (res.idExchShort == 0) {
          // Close Short Bitfinex
          bitfinexOrderId = sendBitfinexOrder(curl, params, "buy", fabs(btcUsed[res.idExchShort]), btcVec[res.idExchShort]->getLastAsk());
        }
        else if (res.idExchShort == 1) {
          // Close Short OkCoin
          // To be implemented
        }
        
        // wait for the orders to be filled
        sleep(3.0);
	std::cout << "Waiting for the two orders to be filled..." << std::endl;
	while (!isBitfinexOrderComplete(curl, params, bitfinexOrderId) || !isOkCoinOrderComplete(curl, params, okCoinOrderId)) {
	  sleep(3.0);
	}
	std::cout << "Done\n" << std::endl;
        bitfinexOrderId = 0;
	okCoinOrderId = 0;

        // market exited
        inMarket = false;
        
        // new balances
        newBalUsd[0] = getBitfinexAvail(curl, params, "usd");  
        newBalUsd[1] = getOkCoinAvail(curl, params, "usd");
        
        newBalBtc[0] = getBitfinexAvail(curl, params, "btc");
        newBalBtc[1] = getOkCoinAvail(curl, params, "btc");
        
        for (unsigned i = 0; i < params.nbExch(); ++i) {
          std::cout << "New balance on " << params.exchName[i] << ":  \t";
          std::cout << newBalUsd[i] << " USD (perf $" << newBalUsd[i] - balanceUsd[i] << "), ";
	  std::cout << std::setprecision(6) << newBalBtc[i]  << std::setprecision(2) << " BTC" << std::endl;
        }
        std::cout << std::endl;

        // update res with total balance
        res.befBalUsd = std::accumulate(balanceUsd.begin(), balanceUsd.end(), 0.0);
        res.aftBalUsd = std::accumulate(newBalUsd.begin(), newBalUsd.end(), 0.0);

        // update current balances with new values
        for (unsigned i = 0; i < params.nbExch(); ++i) {
          balanceUsd[i] = newBalUsd[i];
          balanceBtc[i] = newBalBtc[i];
        }

        // display performance
        std::cout << "ACTUAL PERFORMANCE: " << "$" << res.aftBalUsd - res.befBalUsd << " (" << res.totPerf() * 100.0 << "%)\n" << std::endl; 
        
        // new csv line with result
        csvFile << res.id << "," << res.exchNameLong << "," << res.exchNameShort << "," << printDateTimeCsv(res.entryTime) << "," << printDateTimeCsv(res.exitTime);
        csvFile << "," << res.getLength() << "," << res.exposure * 2.0 << "," << res.befBalUsd << "," << res.aftBalUsd << "," << res.totPerf() << "\n";
        csvFile.flush();

        // send email
	if (params.sendEmail) {
	  sendEmail(res, params);
	  std::cout << "Email sent" << std::endl;
        }

        // delete result information
        res.clear();
        
        // if "stop_after_exit" file exists then return
        std::ifstream infile("stop_after_exit");
        if (infile.good()) {
          std::cout << "Exit after last trade (file stop_after_exit found)" << std::endl;
          // close cURL
          curl_easy_cleanup(curl);
          curl_global_cleanup();
          // close csv file
          csvFile.close(); 
          return 0;
        }
      }
      if (params.verbose) {
        std::cout << std::endl;
      }
    }
    // activities for this iteration terminated
    // update the timeinfo structure
    timeinfo->tm_sec = timeinfo->tm_sec + gapSec;
    currIteration++;
  }
  std::cout << "Max iteration reached (" << debugMaxIteration << ")" <<std::endl;
  // close cURL
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  // close csv file
  csvFile.close();
  

  return 0;
}
