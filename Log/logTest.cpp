#include "Log.hpp"

int main() {
  log.mark("Hello World");
  log(__FILE__, __LINE__, __func__, "Hello World");

  log.mark("All Test");
  log(__FILE__, __LINE__, __func__, "ALL TEST", ALL);

  log.mark("INFILE TEST");
  log(__FILE__, __LINE__, __func__, "INFILE TEST", INFILE);

  log.mark("CONSOLE TEST");
  log(__FILE__, __LINE__, __func__, "CONSOLE TEST", CONSOLE);
}
