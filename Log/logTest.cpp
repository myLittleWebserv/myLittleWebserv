#include "Log.hpp"

int main() {
  Log::log().mark("Hello World");
  Log::log()(__FILE__, __LINE__, __func__, "Hello World");

  Log::log().mark("All Test");
  Log::log()(__FILE__, __LINE__, __func__, "ALL TEST", ALL);

  Log::log().mark("INFILE TEST");
  Log::log()(__FILE__, __LINE__, __func__, "INFILE TEST", INFILE);

  Log::log().mark("CONSOLE TEST");
  Log::log()(__FILE__, __LINE__, __func__, "CONSOLE TEST", CONSOLE);
}
