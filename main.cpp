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
  MyThreadWrapper(std::shared_ptr<std::thread> thread)
  : thread_(thread) {
    
  }
  ~MyThreadWrapper() {
    thread_->join();
  }
  std::shared_ptr<std::thread> operator->() {
    return thread_;
  }
  
private:
  std::shared_ptr<std::thread> thread_;
};

struct Message {
  enum MsgType {
    DATA = 0,
    NEED_RESULTS = 1
  };
  
  Message(MsgType tmp_type, INT tmp_data = 0) :
  type(tmp_type), data(tmp_data) {};
  
  MsgType type;
  INT data;
};

class MsgQueue {
public:
  static MsgQueue* instance() {
    if (!instance_)
      instance_ = std::shared_ptr<MsgQueue>(new MsgQueue());
    return instance_.get();
  }
  
  std::queue<std::shared_ptr<Message>>& msg_queue() { return msg_queue_; };
  std::mutex& mutex() { return mutex_; };
  std::condition_variable& cond() { return cond_; };
private:
  MsgQueue() {};
  
  static std::shared_ptr<MsgQueue> instance_;
  std::queue<std::shared_ptr<Message>> msg_queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

std::shared_ptr<MsgQueue> MsgQueue::instance_ = nullptr;

class PrimeCollector {
public:
  void Run() {
    while (true) {
      std::unique_lock<std::mutex> lk(MsgQueue::instance()->mutex());
//      std::cout << "lock" << std::endl;

      if (!MsgQueue::instance()->msg_queue().empty()) {
        std::cout << "it's not empty!" << std::endl;
        std::shared_ptr<Message> pMsg = MsgQueue::instance()->msg_queue().front();
        MsgQueue::instance()->msg_queue().pop();
        // handle_that_string(s);
      } else {
        MsgQueue::instance()->cond().wait(lk);
//        std::cout << "waiting" << std::endl;
      }
    }
  }
  
  void Factorize(INT number) {
    std::cout << "nubmer: " << number << std::endl;
    primes_pow_.clear();
    
    INT z = 2;
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
    for (auto prime : primes_pow_) {
      std::cout << prime.first << "^" << prime.second << ", ";
    }
    std::cout << std::endl;
  }
  void PrintResult() {
        std::cout << "result: ";
    for (auto prime : primes_max_pow_) {
      std::cout << prime.first << "^" << prime.second << ", ";
    }
    std::cout << std::endl;
  }
  
private:
  void AddPrime(uint16_t prime) {
    if (primes_pow_.find(prime) == primes_pow_.end())
      primes_pow_[prime] = 0;
    primes_pow_[prime]++;
  }
  
  void UpdateMaxPows() {
    for (auto prime : primes_pow_) {
      if (primes_max_pow_.find(prime.first) == primes_max_pow_.end()) {
        primes_max_pow_[prime.first] = prime.second;
        continue;
      }
      if (prime.second > primes_max_pow_[prime.first])
        primes_max_pow_[prime.first] = prime.second;
    }
  }

  std::map<INT, INT> primes_pow_;
  std::map<INT, INT> primes_max_pow_;
};

int main(int argc, const char * argv[]) {
  
  int threads_number = 9;// TODO: ask user?
  
  std::cout << "hi" << std::endl;
  
  std::list<std::shared_ptr<MyThreadWrapper>> threads_list;
  std::list<std::shared_ptr<PrimeCollector>> primes_collectors_list;
  
  for (int i = 0; i < threads_number; ++i) {
    std::shared_ptr<PrimeCollector> prime_collector =
    std::shared_ptr<PrimeCollector>(new PrimeCollector());
    std::shared_ptr<MyThreadWrapper> pThread =
    std::shared_ptr<MyThreadWrapper>(new MyThreadWrapper(std::shared_ptr<std::thread>(new std::thread(&PrimeCollector::Run, prime_collector))));
    primes_collectors_list.push_back(prime_collector);
    threads_list.push_back(pThread);
  }
  
  while (true) {
    INT number;
    std::cin >> number;
    if (number == 2)
      break;
    
    std::string msg = "asd";
    {
      std::unique_lock<std::mutex> lk(MsgQueue::instance()->mutex());
      std::cout << "push msg" << std::endl;
      MsgQueue::instance()->msg_queue().push(std::shared_ptr<Message>(new Message(Message::DATA, number)));
    }
    MsgQueue::instance()->cond().notify_one();
  }
  
  std::cout << "ending..." << std::endl;

  return 0;
}




