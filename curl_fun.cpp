#include "curl_fun.h"
#include <unistd.h>
#include <iostream>

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}


json_t* getJsonFromUrl(CURL* curl, std::string url, std::string postFields) {

  // JSON infomation
  json_t *root;
  json_error_t error;

  // data in readBuffer
  CURLcode resCurl;
  std::string readBuffer;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  if (!postFields.empty()) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  resCurl = curl_easy_perform(curl);

  while (resCurl != CURLE_OK) {
    std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
    sleep(2.0);
    readBuffer = "";
    resCurl = curl_easy_perform(curl);
  }
  root = json_loads(readBuffer.c_str(), 0, &error);
  
  while (!root) {
    std::cout << "Error with JSON:\n" << error.text << ". Retrying..." << std::endl;
    readBuffer = "";
    resCurl = curl_easy_perform(curl); 
    while (resCurl != CURLE_OK) {
      std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
  }
  return root;
}


