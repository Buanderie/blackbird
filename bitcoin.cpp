#include "bitcoin.h"
#include <math.h>

Bitcoin::Bitcoin(unsigned i, std::string n, double f, bool h) {
  id = i;
  exchName = n;
  fees = f;
  hasShort = h;
  std::vector<time_t> *d = new std::vector<time_t>;
  std::vector<double> *b = new std::vector<double>;
  std::vector<double> *a = new std::vector<double>;
  std::vector<double> *v = new std::vector<double>;
  datetime = d;
  bid = b;
  ask = a;
  volume = v;
}


Bitcoin::Bitcoin(unsigned i, std::string n, double f, bool h, std::vector<time_t> *dt, std::vector<double> *b, std::vector<double> *a, std::vector<double> *v) {
  id = i;
  exchName = n;
  fees = f;
  hasShort = h;  
  datetime = dt;
  bid = b;
  ask = a;
  volume = v;
}


Bitcoin::~Bitcoin() {
  delete(datetime);
  delete(bid);
  delete(ask);
  delete(volume);
}


void Bitcoin::addData(time_t d, double b, double a, double v) {
  datetime->push_back(d);
  bid->push_back(b);
  ask->push_back(a);
  volume->push_back(v);
}

void Bitcoin::removeLastData() {
  bid->pop_back();
  ask->pop_back();
  volume->pop_back();
}

unsigned Bitcoin::getId() const {
  return id;
}

time_t Bitcoin::getLastTime_t() const {
  return datetime->back();
}

time_t Bitcoin::getTime_t(int i) const {
  return (*datetime)[i];
}

std::vector<time_t> Bitcoin::getDatetime() const {
  return *datetime;
}

int Bitcoin::getDatetimeSize() const {
  return datetime->size();
}

double Bitcoin::getBid(int i) const {
  return (*bid)[i];
}

std::vector<double> Bitcoin::getBid() const {
  return *bid;
}

double Bitcoin::getLastBid() const {
  if (bid->empty()) {
    return 0;
  }
  else {
    return bid->back();
  }
}

double Bitcoin::getAsk(int i) const {
  return (*ask)[i];
}

std::vector<double> Bitcoin::getAsk() const {
  return *ask;
}

double Bitcoin::getLastAsk() const {
  if (ask->empty()) {
    return 0;
  }
  else {
    return ask->back();
  }
}

double Bitcoin::getPrice(int i) const {
  return ((*bid)[i] + (*ask)[i]) / 2;
}

double Bitcoin::getLastPrice() const {
  if (bid->empty() || ask->empty()) {
    return 0;
  }
  else {
    return (bid->back() + ask->back()) / 2;
  }
}

double Bitcoin::getVolume(int i) const {
  return (*volume)[i];
}

std::vector<double> Bitcoin::getVolume() const {
  return *volume;
}

std::string Bitcoin::getExchName() const {
  return exchName;
}

double Bitcoin::getFees() const {
  return fees;
}

bool Bitcoin::getHasShort() const {
  return hasShort;
}

