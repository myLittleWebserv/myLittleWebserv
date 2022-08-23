#include <iostream>

#include "Router/Router.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "please check arguments" << std::endl;
    return 1;
  }

  Router router(argv[1]);

  while (1) {
    try {
      router.start();
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Server Reoot...!" << std::endl;
    }
  }
  return 0;
}
