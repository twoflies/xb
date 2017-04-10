/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <string>
#include <vector>
#include <pthread.h>

#include "../xbserial/serial.h"

namespace XB {

  typedef int (*MonitorCallback)(ResponseFrame* frame);

  typedef std::pair<Command, Parameter> CommandParameter;

  struct ModuleConfiguration {
    std::string identifier;
    std::vector<CommandParameter> commandParameters;
  };
  
  class Manager {
  public:
    Manager();
    ~Manager();
    int discoverModules(std::vector<Module*>& modules);
    int configureModule(Module* module, ModuleConfiguration* configuration);
    int setModuleIdentifier(Module* module, char* identifier);
    
    int startMonitoring(MonitorCallback callback);
    int stopMonitoring();

  private:
    static void* monitor_(void* context);
    void* monitor();

  private:
    Serial serial_;
    pthread_t monitorThread_;
    pthread_mutex_t monitorMutex_;
    MonitorCallback monitorCallback_;
  };
  
}

#endif  // _MANAGER_H_
