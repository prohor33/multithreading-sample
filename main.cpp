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
#include <queue>

#define INT uint64_t

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

class MsgQueue {
public:
  static MsgQueue* Instance() {
    if (!m_pInstance)
      m_pInstance = std::shared_ptr<MsgQueue>(new MsgQueue());
    return m_pInstance.get();
  }
  
  std::queue<std::string> m_MsgQueue;
  std::mutex m_Mutex;
  std::condition_variable m_Cond;
  
private:
  MsgQueue() {};
  
  static std::shared_ptr<MsgQueue> m_pInstance;
};

std::shared_ptr<MsgQueue> MsgQueue::m_pInstance = nullptr;

class PrimeCollector {
public:
  void Run() {
    while (true) {
      std::unique_lock<std::mutex> lk(MsgQueue::Instance()->m_Mutex);
//      std::cout << "lock" << std::endl;

      if (!MsgQueue::Instance()->m_MsgQueue.empty()) {
        std::cout << "it's not empty!" << std::endl;
        std::string s = MsgQueue::Instance()->m_MsgQueue.front();
        std::cout << "s = " << s << std::endl;
        MsgQueue::Instance()->m_MsgQueue.pop();
        // handle_that_string(s);
      } else {
        MsgQueue::Instance()->m_Cond.wait(lk);
//        std::cout << "waiting" << std::endl;
      }
    }
  }
  
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

  std::map<INT, INT> m_mPrimesPow;
  std::map<INT, INT> m_mPrimesMaxPow;
};

int main(int argc, const char * argv[]) {
  
  int threads_number = 9;// TODO: ask user?
  
  std::cout << "hi" << std::endl;
  
  std::list<std::shared_ptr<MyThreadWrapper>> lThreads;
  std::list<std::shared_ptr<PrimeCollector>> lPrimesCollectors;
  
  for (int i = 0; i < threads_number; ++i) {
    std::shared_ptr<PrimeCollector> pPrimeCollector =
    std::shared_ptr<PrimeCollector>(new PrimeCollector());
    std::shared_ptr<MyThreadWrapper> pThread =
    std::shared_ptr<MyThreadWrapper>(new MyThreadWrapper(std::shared_ptr<std::thread>(new std::thread(&PrimeCollector::Run, pPrimeCollector))));
    lPrimesCollectors.push_back(pPrimeCollector);
    lThreads.push_back(pThread);
  }
  
  while (true) {
    INT number;
    std::cin >> number;
    if (number == 2)
      break;
    
    std::string msg = "asd";
    {
      std::unique_lock<std::mutex> lk(MsgQueue::Instance()->m_Mutex);
      std::cout << "push msg" << std::endl;
      MsgQueue::Instance()->m_MsgQueue.push(msg);
    }
    MsgQueue::Instance()->m_Cond.notify_one();
  }
  
  std::cout << "ending..." << std::endl;

  return 0;
}




