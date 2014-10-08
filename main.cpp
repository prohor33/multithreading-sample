//
//  main.cpp
//  multithreading-sample
//
//  Created by Prohor Gladkikh on 08/10/14.
//  Copyright (c) 2014 Prohor. All rights reserved.
//

#include <iostream>
#include <thread>
#include <map>
#include <list>

class MyThreadWrapper {
public:
  MyThreadWrapper(std::shared_ptr<std::thread> pThread)
  : m_pThread(pThread) {
    
  }
  ~MyThreadWrapper() {
    m_pThread->join();
  }
  std::shared_ptr<std::thread> operator->() {
    return m_pThread;
  }
  
private:
  std::shared_ptr<std::thread> m_pThread;
};

class PrimeCollector {
public:
  void Factorize(uint16_t number) {
    std::cout << "nubmer: " << number << std::endl;
    m_mPrimesPow.clear();
    
    uint16_t z = 2;
    while (z * z <= number) {
      if (number % z == 0) {
//        std::cout << z << std::endl;
        AddPrime(z);
        number /= z;
      } else {
        z++;
      }
    }
    
    if (number > 1)
      AddPrime(number);
    
    PrintTempResult();
    UpdateMaxPows();
    PrintResult();
  }
  
  void PrintTempResult() {
    std::cout << "temp result: ";
    for (auto prime : m_mPrimesPow) {
      std::cout << prime.first << "^" << prime.second << ", ";
    }
    std::cout << std::endl;
  }
  void PrintResult() {
        std::cout << "result: ";
    for (auto prime : m_mPrimesMaxPow) {
      std::cout << prime.first << "^" << prime.second << ", ";
    }
    std::cout << std::endl;
  }
  
private:
  void AddPrime(uint16_t prime) {
    if (m_mPrimesPow.find(prime) == m_mPrimesPow.end())
      m_mPrimesPow[prime] = 0;
    m_mPrimesPow[prime]++;
  }
  
  void UpdateMaxPows() {
    for (auto prime : m_mPrimesPow) {
      if (m_mPrimesMaxPow.find(prime.first) == m_mPrimesMaxPow.end()) {
        m_mPrimesMaxPow[prime.first] = prime.second;
        continue;
      }
      if (prime.second > m_mPrimesMaxPow[prime.first])
        m_mPrimesMaxPow[prime.first] = prime.second;
    }
  }

  std::map<uint64_t, uint64_t> m_mPrimesPow;
  std::map<uint64_t, uint64_t> m_mPrimesMaxPow;
};

int main(int argc, const char * argv[]) {
  
  int threads_number = 9;// TODO: ask user?
  
  std::cout << "hi" << std::endl;
  
  std::list<std::shared_ptr<MyThreadWrapper>> lThreads;
  std::list<std::shared_ptr<PrimeCollector>> lPrimesCollectors;
  
  while (true) {
    uint16_t number;
    std::cin >> number;
    if (number == 2)
      break;
    
    std::shared_ptr<PrimeCollector> pPrimeCollector =
    std::shared_ptr<PrimeCollector>(new PrimeCollector());
    std::shared_ptr<MyThreadWrapper> pThread =
    std::shared_ptr<MyThreadWrapper>(new MyThreadWrapper(
        std::shared_ptr<std::thread>(new std::thread(&PrimeCollector::Factorize, pPrimeCollector, number))));
    lPrimesCollectors.push_back(pPrimeCollector);
    lThreads.push_back(pThread);
  }
  

//  t1.join();
    std::cout << "ending..." << std::endl;

  return 0;
}