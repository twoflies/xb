/*********************************************************************/
/* xbm                                                               */
/*********************************************************************/

#include "xbm.h"

#include <vector>
#include <iostream>
#include <string>

#include "../xbserial/frame.h"
#include "../xbserial/log.h"

int monitorCallback(XB::Frame *frame) {
  using namespace XB;

  return 0;
}

int main (int argc, char** argv) {
  using namespace XB;

  Manager manager;
  int result = manager.initialize();
  if (result != 0) {
    return result;
  }
  
  std::vector<Module*> modules;
  result = manager.discoverModules(modules);
  if (result != 0) {
    return result;
  }

  ModuleConfiguration configuration;
  configuration.addCommandParameter(Command("V+"), Parameter(0xFFFF));
  configuration.addCommandParameter(Command("IR"), Parameter(0x32));
  configuration.addCommandParameter(Command("SP"), Parameter(0xC8));
  //configuration.addCommandParameter(Command("SN"), Parameter(0x14));
  configuration.addCommandParameter(Command("ST"), Parameter(0x32));

  for (std::vector<Module*>::iterator it = modules.begin(); it != modules.end(); it++) {
    Module* module = *it;
    if (module->identifier.find_first_not_of(' ') == std::string::npos) {
      log("Configuring module '%s'", module->identifier.c_str());
      result = manager.configureModule(module, &configuration);
      if (result != 0) {
	logError(result, "Failed to configure module '%s'", module->identifier.c_str());
      }
    }
  }

  for (std::string line; std::getline(std::cin, line);) {
    if (line.compare("q") == 0) {
      break;
    }
  }

  return 0;
}
