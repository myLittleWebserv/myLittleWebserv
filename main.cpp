#include <iostream>

#include "Router/Router.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "please check arguments" << std::endl;
    return 1;
  }

  Router router(argv[1]);

  // while (1) {
  try {
    router.start();
  } catch (const std::string& error_message) {
    std::cerr << "error : " << error_message << std::endl;
    std::cerr << "Server Reoot...!" << std::endl;
  }
  // }
  return 0;
}
