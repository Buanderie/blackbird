#include <stdlib.h>
#include "check_entry_exit.h"

bool checkEntry(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters params) {

  double priceLong;
  double priceShort;
  
  // compute limit (fees + spreadEntry)
  double limit = 2 * btcLong->getFees() + 2 * btcShort->getFees() + params.spreadEntry;

  // check if btcLong is below btcShort
  if (btcShort->getHasShort()) {
   
    // btcLong:  buy looking to match ask
    // btcShort: sell looking to match bid
    priceLong = btcLong->getLastAsk();
    priceShort = btcShort->getLastBid();
    res.spreadIn = (priceShort - priceLong) / priceLong;

    // maxCurrSpread and minCurrSpread
    if (res.spreadIn > res.maxCurrSpread) {
      res.maxCurrSpread = res.spreadIn;
    }
    if (res.spreadIn < res.minCurrSpread) {
      res.minCurrSpread = res.spreadIn;
    }
    if (params.verbose) {
      std::cout << "   Spread " << btcLong->getExchName() << "/" << btcShort->getExchName() << ": " << res.spreadIn * 100.0 << "%";
      std::cout << " (target " << limit * 100.0 << "%, min " << res.minCurrSpread * 100.0 << "%, max " << res.maxCurrSpread * 100.0 << "%)" << std::endl;
    }
    if (res.spreadIn >= limit && priceLong > 0.0 && priceShort > 0.0) {
      // opportunity found
      res.idExchLong = btcLong->getId();
      res.idExchShort = btcShort->getId();
      res.feesLong = btcLong->getFees();
      res.feesShort = btcShort->getFees();
      res.exchNameLong = btcLong->getExchName();
      res.exchNameShort = btcShort->getExchName();
      res.priceLongIn = priceLong;
      res.priceShortIn = priceShort;
      return true;
    }
  }
  // no opportunity
  return false;
}


bool checkExit(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters params, time_t period) {

  double priceLong;
  double priceShort;

  // close btcLong:  sell looking to match bid
  // close btcShort: buy looking to match ask
  priceLong  = btcLong->getLastBid();
  priceShort = btcShort->getLastAsk();
  res.spreadOut = (priceShort - priceLong) / priceLong;
  
  // maxCurrSpread and minCurrSpread
  if (res.spreadOut > res.maxCurrSpread) {
    res.maxCurrSpread = res.spreadOut;
  }
  if (res.spreadOut < res.minCurrSpread) {
    res.minCurrSpread = res.spreadOut;
  }

  if (params.verbose) {
    std::cout << "   Spread " << btcLong->getExchName() << "/" << btcShort->getExchName() << ": " << res.spreadOut * 100.0 << "%";
    std::cout << " (target " << params.spreadExit * 100.0 << "%, min " << res.minCurrSpread * 100.0 << "%, max " << res.maxCurrSpread * 100.0 << "%)" << std::endl;
  }
  
  // check length
  if (period - res.entryTime >= params.maxLength) {
    res.priceLongOut  = priceLong;
    res.priceShortOut = priceShort;
    return true;
  }
  
  if (res.spreadOut <= params.spreadExit && priceLong > 0.0 && priceShort > 0.0) {
    // exit opportunity found
    res.priceLongOut  = priceLong;
    res.priceShortOut = priceShort;
    return true;
  }
  // no exit opportunity
  return false;
}
