#ifndef BITCOIN_H
#define BITCOIN_H

#include <iostream>
#include <string>
#include <vector>


// This class contains all the information
// (fees, bid, ask, etc) for a given exchange
class Bitcoin {

  private:
    unsigned id;
    std::string exchName;
    double fees;
    bool hasShort;
    std::vector<time_t> *datetime;
    std::vector<double> *bid;
    std::vector<double> *ask;
    std::vector<double> *volume;

  public:
    // constructors
    Bitcoin(unsigned id, std::string n, double f, bool h);
    Bitcoin(unsigned id, std::string n, double f, bool h, std::vector<time_t> *dt, std::vector<double> *b, std::vector<double> *a, std::vector<double> *v);
  
    // destructor
    ~Bitcoin();

    void addData(time_t d, double b, double a, double v);
    void removeLastData();
    
    unsigned getId() const;
    
    time_t getLastTime_t() const;
    time_t getTime_t(int i) const;
    std::vector<time_t> getDatetime() const;
    int getDatetimeSize() const;
    
    double getAsk(int i) const;
    std::vector<double> getAsk() const;
    double getLastAsk() const;
    
    double getBid(int i) const;
    std::vector<double> getBid() const;
    double getLastBid() const;    
    
    double getPrice(int i) const;
    double getLastPrice() const;
    
    double getVolume(int i) const;
    std::vector<double> getVolume() const;
    
    std::string getExchName() const;
    
    double getFees() const;
    
    bool getHasShort() const;
 
 };
 
#endif        
