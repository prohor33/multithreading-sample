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

class ThreadWrapper {
public:
  ThreadWrapper(std::shared_ptr<std::thread> thread)
  : thread_(thread) {}

  ~ThreadWrapper() {
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
    TO_FINISH = 1,
    FINISHED = 2
  };

  Message(MsgType tmp_type, INT tmp_data = 0) :
  type(tmp_type), data(tmp_data) {};

  MsgType type;
  INT data;
};

class MsgQueue {
public:
  std::queue<std::shared_ptr<Message>>& msg_queue() { return msg_queue_; };
  std::mutex& mutex() { return mutex_; };
  std::condition_variable& cond() { return cond_; };

private:
  std::queue<std::shared_ptr<Message>> msg_queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

class Telegraph {
public:
  static Telegraph* Instance() {
    {
      std::lock_guard<std::mutex> lk(mutex_);
      if (!instance_)
        instance_ = std::shared_ptr<Telegraph>(new Telegraph());
    }
    return instance_.get();
  }

  MsgQueue* msgs_to_workers() {
    return msgs_to_workers_.get();
  }
  MsgQueue* msgs_to_main() {
    return msgs_to_main_.get();
  }

private:
  Telegraph() :
  msgs_to_workers_(new MsgQueue()),
  msgs_to_main_(new MsgQueue()) {};

  static std::shared_ptr<Telegraph> instance_;
  static std::mutex mutex_;
  std::shared_ptr<MsgQueue> msgs_to_workers_;
  std::shared_ptr<MsgQueue> msgs_to_main_;
};

std::shared_ptr<Telegraph> Telegraph::instance_ = nullptr;
std::mutex Telegraph::mutex_;

class PrimeCollector {
public:
  void Run() {
    while (true) {
      std::unique_lock<std::mutex> lk(
          Telegraph::Instance()->msgs_to_workers()->mutex());

      if (!Telegraph::Instance()->msgs_to_workers()->msg_queue().empty()) {
        std::shared_ptr<Message> pMsg =
            Telegraph::Instance()->msgs_to_workers()->msg_queue().front();

        if (pMsg->type == Message::DATA) {
          Factorize(pMsg->data);
        } else if (pMsg->type == Message::TO_FINISH) {
          NotifyMain(Message::FINISHED);
          return;  // do not pop the message
        }

        Telegraph::Instance()->msgs_to_workers()->msg_queue().pop();
      } else {
        Telegraph::Instance()->msgs_to_workers()->cond().wait(lk);
      }
    }
  }

  void Factorize(INT number) {
    primes_pow_.clear();

    INT z = 2;
    while (z * z <= number) {
      if (number % z == 0) {
        AddPrime(z);
        number /= z;
      } else {
        z++;
      }
    }

    if (number > 1)
      AddPrime(number);

    MergeMaxPows(primes_pow_);
  }

  void PrintResult() const {
    for (auto prime : primes_max_pow_) {
      std::cout << prime.first << "^" << prime.second << " * ";
    }
    std::cout << std::endl;
  }

  void MergeMaxPows(const std::map<INT, INT>& input_primes_pow) {
    for (auto prime : input_primes_pow) {
      if (primes_max_pow_.find(prime.first) == primes_max_pow_.end()) {
        primes_max_pow_[prime.first] = prime.second;
        continue;
      }
      if (prime.second > primes_max_pow_[prime.first])
        primes_max_pow_[prime.first] = prime.second;
    }
  }

  const std::map<INT, INT>& primes_max_pow() const {
    return primes_max_pow_;
  }

private:
  void AddPrime(INT prime) {
    if (primes_pow_.find(prime) == primes_pow_.end())
      primes_pow_[prime] = 0;
    primes_pow_[prime]++;
  }

  void NotifyMain(Message::MsgType msg_type) {
    {
      std::unique_lock<std::mutex> lk(
          Telegraph::Instance()->msgs_to_main()->mutex());
      Telegraph::Instance()->msgs_to_main()->msg_queue().push(
          std::shared_ptr<Message>(new Message(msg_type)));
    }
    Telegraph::Instance()->msgs_to_main()->cond().notify_one();
  }

  std::map<INT, INT> primes_pow_;
  std::map<INT, INT> primes_max_pow_;
};

int main(int argc, const char * argv[]) {

  int threads_number = 100;  // TODO: ask user?

  std::cout << "please, enter your numbers: " << std::endl;

  std::list<std::shared_ptr<ThreadWrapper>> threads_list;
  std::list<std::shared_ptr<PrimeCollector>> primes_collectors_list;

  for (int i = 0; i < threads_number; ++i) {
    std::shared_ptr<PrimeCollector> prime_collector =
    std::shared_ptr<PrimeCollector>(new PrimeCollector());
    std::shared_ptr<ThreadWrapper> pThread =
    std::shared_ptr<ThreadWrapper>(new ThreadWrapper(
            std::shared_ptr<std::thread>(new std::thread(&PrimeCollector::Run,
                                                         prime_collector))));
    primes_collectors_list.push_back(prime_collector);
    threads_list.push_back(pThread);
  }

  while (!std::cin.eof()) {
    int64_t tmp;
    std::cin >> tmp;
    if (tmp < 1) {
      std::cout << "bad input: input number = " << tmp <<
          " is below zero" << std::endl;
      break;
    }
    INT number = static_cast<INT>(tmp);

    {
      std::unique_lock<std::mutex> lk(
          Telegraph::Instance()->msgs_to_workers()->mutex());
      // push data message
      Telegraph::Instance()->msgs_to_workers()->msg_queue().push(
          std::shared_ptr<Message>(new Message(Message::DATA, number)));
    }
    Telegraph::Instance()->msgs_to_workers()->cond().notify_one();
  }

  {
    std::unique_lock<std::mutex> lk(
        Telegraph::Instance()->msgs_to_workers()->mutex());
    //  push comand to terminate
    Telegraph::Instance()->msgs_to_workers()->msg_queue().push(
        std::shared_ptr<Message>(new Message(Message::TO_FINISH)));
  }
  Telegraph::Instance()->msgs_to_workers()->cond().notify_all();

  int threads_terminated = 0;
  while (true) {
    std::unique_lock<std::mutex> lk(
        Telegraph::Instance()->msgs_to_main()->mutex());

    if (!Telegraph::Instance()->msgs_to_main()->msg_queue().empty()) {
      std::shared_ptr<Message> pMsg =
          Telegraph::Instance()->msgs_to_main()->msg_queue().front();
      if (pMsg->type == Message::FINISHED) {
        threads_terminated++;
        if (threads_terminated == threads_number) {
          // all threads have completed their work
          break;
        }
      }
      Telegraph::Instance()->msgs_to_main()->msg_queue().pop();
    } else {
      Telegraph::Instance()->msgs_to_main()->cond().wait(lk);
    }
  }

  //  merge results from different threads
  auto it_first = primes_collectors_list.begin();
  if (it_first == primes_collectors_list.end()) {
    std::cout << "error: no prime collectors" << std::endl;
    return -1;
  }
  auto it = primes_collectors_list.begin();
  it++;
  while (it != primes_collectors_list.end()) {
    (*it_first)->MergeMaxPows((*it)->primes_max_pow());
    ++it;
  }
  (*it_first)->PrintResult();

  return 0;
}




