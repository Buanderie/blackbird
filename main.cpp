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
#include "bitstamp.h"
#include "kraken.h"
#include "send_email.h"

// typedef
typedef double (*getQuoteType) (CURL *curl, bool isBid);
typedef double (*getAvailType) (CURL *curl, Parameters params, std::string currency);
typedef int (*sendOrderType) (CURL *curl, Parameters params, std::string direction, double quantity, double price);
typedef bool (*isOrderCompleteType) (CURL *curl, Parameters params, int orderId);
typedef double (*getActivePosType) (CURL *curl, Parameters params);
typedef double (*getLimitPriceType) (CURL *curl, double volume, bool isBid);

int main(int argc, char **argv) {

  // header information
  std::cout << "Blackbird Bitcoin Arbitrage\nVersion 0.0.2" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK.\n" << std::endl;

  // read the config file (config.json)
  json_error_t error;
  json_t *root = json_load_file("config.json", 0, &error);
  if (!root) {
    std::cout << "ERROR: config.json incorrect (" << error.text << ")\n" << std::endl;
    return 1;
  }
 
  // get config variables
  int gapSec = json_integer_value(json_object_get(root, "GapSec"));
  unsigned  debugMaxIteration = json_integer_value(json_object_get(root, "DebugMaxIteration"));

  bool useFullCash = json_boolean_value(json_object_get(root, "UseFullCash")); 
  double untouchedCash = json_real_value(json_object_get(root, "UntouchedCash"));
  double cashForTesting = json_real_value(json_object_get(root, "CashForTesting"));

  if (!useFullCash && cashForTesting < 15.0) {
    std::cout << "ERROR: Minimum test cash is $15.00.\n" << std::endl;
    return 1;
  }
  
  // function arrays 
  getQuoteType getQuote[] = {Bitfinex::getQuote, OkCoin::getQuote, Bitstamp::getQuote, Kraken::getQuote};
  getAvailType getAvail[] = {Bitfinex::getAvail, OkCoin::getAvail, Bitstamp::getAvail, Kraken::getAvail};
  sendOrderType sendOrder[] = {Bitfinex::sendOrder, OkCoin::sendOrder, Bitstamp::sendOrder};
  isOrderCompleteType isOrderComplete[] = {Bitfinex::isOrderComplete, OkCoin::isOrderComplete, Bitstamp::isOrderComplete};
  getActivePosType getActivePos[] = {Bitfinex::getActivePos, OkCoin::getActivePos, Bitstamp::getActivePos, Kraken::getActivePos};
  getLimitPriceType getLimitPrice[] = {Bitfinex::getLimitPrice, OkCoin::getLimitPrice, Bitstamp::getLimitPrice, Kraken::getLimitPrice};

  // thousand separator
  std::locale mylocale("");
  std::cout.imbue(mylocale);

  // print precision of two digits
  std::cout.precision(2);
  std::cout << std::fixed;

  // create a parameters structure
  Parameters params(root);
  params.addExchange("Bitfinex", 0.0020, true);
  params.addExchange("OKCoin", 0.0020, false);
  params.addExchange("Bitstamp", 0.0025, false);
  params.addExchange("Kraken", 0.0025, true);

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

  // create Bitcoin objects
  for (unsigned i = 0; i < params.nbExch(); ++i) {
    btcVec.push_back(new Bitcoin(i, params.exchName[i], params.fees[i], params.hasShort[i]));
  }

  // cURL
  CURL* curl;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  std::cout << "[ Targets ]" << std::endl;
  std::cout << "   Spread to enter: " << params.spreadEntry * 100.0 << "%" << std::endl;
  std::cout << "   Spread to exit: " << params.spreadExit * 100.0  << "%" << std::endl;
  std::cout << std::endl;

  // store current balances
  std::cout << "[ Current balances ]" << std::endl;
  std::vector<double> balanceUsd;
  std::vector<double> balanceBtc;
 
  for (unsigned i = 0; i < params.nbExch(); ++i) {
    balanceUsd.push_back(getAvail[i](curl, params, "usd"));
    balanceBtc.push_back(getAvail[i](curl, params, "btc"));
  }

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
    std::cout << "   FULL cash used!" << std::endl;
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

  // main loop
  if (!params.verbose) {
    std::cout << "Running..." << std::endl;  
  }

  unsigned currIteration = 0;
  bool stillRunning = true;
  while (stillRunning) {

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

      double bid = getQuote[e](curl, true);
      double ask = getQuote[e](curl, false);
      if (params.verbose) {
        std::cout << "   " << params.exchName[e] << ": \t" << bid << " / " << ask << std::endl;
      }
      btcVec[e]->addData(currTime, bid, ask, 0.0);
      curl_easy_reset(curl);
    }
    // load data terminated
    if (params.verbose) {
      std::cout << "   ----------------------------" << std::endl;
    }
    // compute entry point
    if (!inMarket) {
      for (unsigned i = 0; i < params.nbExch(); ++i) {
        for (unsigned j = 0; j < params.nbExch(); ++j) {
          if (i != j) {
            if (checkEntry(btcVec[i], btcVec[j], res, params)) {
              // entry opportunity found
              // compute exposure
              res.exposure = std::min(balanceUsd[res.idExchLong], balanceUsd[res.idExchShort]);
              if (res.exposure == 0.0) {
                std::cout << "   WARNING: Opportunity found but no cash available. Trade canceled." << std::endl;
                break;
              }
              if (useFullCash == false && res.exposure <= cashForTesting) {
                std::cout << "   WARNING: Opportunity found but no enough cash. Need more than TEST cash (min. $" << cashForTesting << "). Trade canceled." << std::endl;
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

	      // check volumes
	      double volumeLong = res.exposure / btcVec[res.idExchLong]->getLastAsk();
	      double volumeShort = res.exposure / btcVec[res.idExchShort]->getLastBid();
	      double limPriceLong = getLimitPrice[res.idExchLong](curl, volumeLong, false);	
	      double limPriceShort = getLimitPrice[res.idExchShort](curl, volumeShort, true);
	      
	      if (limPriceLong - res.priceLongIn > 0.30 || res.priceShortIn - limPriceShort > 0.30) {
                std::cout << "   WARNING: Opportunity found but not enough volume. Trade canceled." << std::endl;
		break;
	      }

              inMarket = true;
              resultId++;
              // update result
              res.id = resultId;
              res.entryTime = currTime;
              res.printEntry();
              res.maxSpread[res.idExchLong][res.idExchShort] = -1.0;
              res.minSpread[res.idExchLong][res.idExchShort] = 1.0;
              int longOrderId = 0;
              int shortOrderId = 0;

              // send Long order
              longOrderId = sendOrder[res.idExchLong](curl, params, "buy", volumeLong, btcVec[res.idExchLong]->getLastAsk());
              // send Short order
              shortOrderId = sendOrder[res.idExchShort](curl, params, "sell", volumeShort, btcVec[res.idExchShort]->getLastBid());
              
              // wait for the orders to be filled
              sleep(3.0);
              std::cout << "Waiting for the two orders to be filled..." << std::endl;
              while (!isOrderComplete[res.idExchLong](curl, params, longOrderId) || !isOrderComplete[res.idExchShort](curl, params, shortOrderId)) {
                sleep(3.0);
              }
              std::cout << "Done" << std::endl;
              longOrderId = 0;
              shortOrderId = 0;
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
        // exit opportunity found


        // check current exposure
        std::vector<double> btcUsed;
        for (unsigned i = 0; i < params.nbExch(); ++i) {
          btcUsed.push_back(getActivePos[i](curl, params));
        }  
       
        // check volumes
	double volumeLong = btcUsed[res.idExchLong];
	double volumeShort = btcUsed[res.idExchShort];
        double limPriceLong = getLimitPrice[res.idExchLong](curl, volumeLong, true);
        double limPriceShort = getLimitPrice[res.idExchShort](curl, volumeShort, false);
        if (res.priceLongOut - limPriceLong > 0.30 || limPriceShort - res.priceShortOut > 0.30) {
          std::cout << "   WARNING: Opportunity found but not enough volume. Trade canceled." << std::endl;
	} else {
	  res.exitTime = currTime;
          res.printExit();
          // send orders
          int longOrderId = 0;
          int shortOrderId = 0;
          std::cout << std::setprecision(6) << "BTC exposure on " << params.exchName[res.idExchLong] << ": " << volumeLong << std::setprecision(2) << std::endl;
          std::cout << std::setprecision(6) << "BTC exposure on " << params.exchName[res.idExchShort] << ": " << volumeShort << std::setprecision(2) << std::endl;
          std::cout << std::endl;
        
          // Close Long
          longOrderId = sendOrder[res.idExchLong](curl, params, "sell", fabs(btcUsed[res.idExchLong]), btcVec[res.idExchLong]->getLastBid());
          // Close Short
          shortOrderId = sendOrder[res.idExchShort](curl, params, "buy", fabs(btcUsed[res.idExchShort]), btcVec[res.idExchShort]->getLastAsk());
        
          // wait for the orders to be filled
          sleep(3.0);
          std::cout << "Waiting for the two orders to be filled..." << std::endl;
          while (!isOrderComplete[res.idExchLong](curl, params, longOrderId) || !isOrderComplete[res.idExchShort](curl, params, shortOrderId)) {
            sleep(3.0);
          }
          std::cout << "Done\n" << std::endl;
          longOrderId = 0;
          shortOrderId = 0;

          // market exited
          inMarket = false;
        
          // new balances
          for (unsigned i = 0; i < params.nbExch(); ++i) {
            newBalUsd[i] = getAvail[i](curl, params, "usd");
            newBalBtc[i] = getAvail[i](curl, params, "btc");
          }
        
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
            stillRunning = false;
          }
        }
      }
      if (params.verbose) {
        std::cout << std::endl;
      }
    }
    // activities for this iteration terminated
    timeinfo->tm_sec = timeinfo->tm_sec + gapSec;
    currIteration++;
    if (currIteration >= debugMaxIteration) {
      std::cout << "Max iteration reached (" << debugMaxIteration << ")" <<std::endl;
      stillRunning = false;
    }
  }
  // delete Bitcoin objects
  for (unsigned i = 0; i < params.nbExch(); ++i) {
    delete(btcVec[i]);
  }
  json_decref(root);
  // close cURL
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  // close csv file
  csvFile.close();
  
  return 0;
}
