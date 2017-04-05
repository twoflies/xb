/*********************************************************************/
/* xbm                                                               */
/*********************************************************************/

#include "xbm.h"

#include <iostream>
#include <string>

int monitorCallback(XB::ResponseFrame *frame) {
  return 0;
}

int discoverCallback(XB::Module *module) {

  return 0;
}

int main (int argc, char** argv) {
  using namespace XB;

  Manager manager;
  /*int result = manager.monitor(&monitorCallback);
  if (result != 0) {
    return result;
    }*/
  
  int result = manager.discover(&discoverCallback);
  if (result != 0) {
    return result;
  }

  for (std::string line; std::getline(std::cin, line);) {
    if (line.compare("q") == 0) {
      break;
    }
  }

  return 0;
}
