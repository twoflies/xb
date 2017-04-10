/*********************************************************************/
/* xbm                                                               */
/*********************************************************************/

#include "xbm.h"

#include <vector>
#include <iostream>
#include <string>

#include "../xbserial/frame.h"
#include "../xbserial/log.h"

int monitorCallback(XB::ResponseFrame *frame) {
  using namespace XB;

  return 0;
}

int main (int argc, char** argv) {
  using namespace XB;

  Manager manager;
  
  /*std::vector<Module*> modules;
  int result = manager.discoverModules(modules);
  if (result != 0) {
    return result;
  }

  ModuleConfiguration configuration;
  configuration.commandParameters.push_back(std::make_pair(Command("IC"), Parameter(0x0001)));
  configuration.commandParameters.push_back(std::make_pair(Command("IR"), Parameter(0x0AF0)));
  configuration.commandParameters.push_back(std::make_pair(Command("SP"), Parameter(0x0AF0)));
  configuration.commandParameters.push_back(std::make_pair(Command("ST"), Parameter(0x0001)));

  for (std::vector<Module*>::iterator it = modules.begin(); it != modules.end(); it++) {
    Module* module = *it;
    if (module->identifier.find_first_not_of(' ') == std::string::npos) {
      log("Configuring module '%s'", module->identifier.c_str());
      result = manager.configureModule(*it, &configuration);
      if (result != 0) {
	logError(result, "Failed to configure module '%s'", module->identifier.c_str());
      }
    }
    }*/

  for (std::string line; std::getline(std::cin, line);) {
    if (line.compare("q") == 0) {
      break;
    }
    else if (line.compare("+") == 0) {
      manager.startMonitoring(&monitorCallback);
    }
    else if (line.compare("-") == 0) {
      manager.stopMonitoring();
    }
  }

  return 0;
}
