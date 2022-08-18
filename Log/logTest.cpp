#include "Log.hpp"



int main() {
    Log& log = Log::getInstance();

    log(std::string(__FILE__), std::string(__func__), __LINE__, std::string("hello world"));
}