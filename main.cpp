//
//  main.cpp
//  multithreading-sample
//
//  Created by Prohor Gladkikh on 08/10/14.
//  Copyright (c) 2014 Prohor. All rights reserved.
//

#include <iostream>
#include <thread>

void call_from_thread() {
  std::cout << "Hello, World" << std::endl;
}

int main(int argc, const char * argv[]) {
  std::thread t1(call_from_thread);
  
  t1.join();

  return 0;
}