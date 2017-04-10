/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#include "manager.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../xbserial/log.h"

namespace XB {

  Manager::Manager() {
    monitorCallback_ = NULL;
    
    serial_.open("/dev/ttyUSB0", 9600);
    
    pthread_mutex_init(&monitorMutex_, NULL);
  }
  
  Manager::~Manager() {
    serial_.close();
    
    pthread_mutex_destroy(&monitorMutex_);
  }

  int Manager::discoverModules(std::vector<Module*>& modules) {
    Parameter parameter;
    int result = serial_.getParameter(Command("NT"), &parameter);
    if (result != 0) {
      return result;
    }

    result = serial_.send(CommandFrame(Command("ND"), 0x01));
    if (result != 0) {
      return result;
    }

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    short timeRemaining = parameter.ushort() * 100;
    while (timeRemaining > 0) {
      ResponseFrame* response = serial_.receiveAny(0, timeRemaining);
      CommandResponseFrame* commandResponse = dynamic_cast<CommandResponseFrame*>(response);
      if (commandResponse != NULL) {
	byte* data = commandResponse->getParameter().data;
	int length = commandResponse->getParameter().length;

	if (length > (int)(sizeof(Address16) + sizeof(Address64))) {
	  Module* module = new Module();
	  module->address16 = Address16(data);
	  module->address64 = Address64(data + sizeof(module->address16));
	  module->identifier = std::string((char*)(data + sizeof(module->address16) + sizeof(module->address64)));

	  modules.push_back(module);
	}
      }
      delete response;

      struct timespec current;
      clock_gettime(CLOCK_MONOTONIC, &current);
      unsigned long long diff = (1000000000 * (current.tv_sec - start.tv_sec) + current.tv_nsec - start.tv_nsec) / 1000000;
      timeRemaining -= diff;
    }
    
    return 0;
  }

  int Manager::configureModule(Module* module, ModuleConfiguration* configuration) {
    char* identifier;
    if (configuration->identifier.empty()) {
      if (asprintf(&identifier, "Node %02X-%02X", module->address16.a, module->address16.b) < 0) {
	return -1;
      }
    }
    else {
      identifier = strdup(configuration->identifier.c_str());
    }

    int result = setModuleIdentifier(module, identifier);
    free(identifier);
    if (result != 0) {
      return result;
    }

    for (std::vector<CommandParameter>::iterator it = configuration->commandParameters.begin(); it != configuration->commandParameters.end(); it++) {
      result = serial_.setRemoteParameter(module, it->first, it->second);
      if (result != 0) {
	return result;
      }
    }

    if (!configuration->commandParameters.empty()) {
      serial_.setRemoteParameter(module, Command("AC"), Parameter());
    }

    return 0;
  }

  int Manager::setModuleIdentifier(Module* module, char* identifier) {
    return serial_.setRemoteParameter(module, Command("NI"), Parameter(identifier));
  }

  int Manager::startMonitoring(MonitorCallback callback) {
    if (callback == NULL) {
      return -1;
    }
    
    int result = pthread_mutex_lock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    int threadResult = -1;
    if (monitorCallback_ == NULL) {
      monitorCallback_ = callback;
      threadResult = pthread_create(&monitorThread_, NULL, &Manager::monitor_, this);
    }
      
    result = pthread_mutex_unlock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    return threadResult;
  }

  int Manager::stopMonitoring() {
    int result = pthread_mutex_lock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    int threadResult = -1;
    if (monitorCallback_ != NULL) {
      monitorCallback_ = NULL;
      threadResult = pthread_cancel(monitorThread_);
    }
    
    result = pthread_mutex_unlock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    if (threadResult == 0) {
      threadResult = pthread_join(monitorThread_, NULL);
      if (threadResult == 0) {
	log("Monitoring stopped.");
      }
    }

    return threadResult;
  }

  void* Manager::monitor_(void *context) {
    return ((Manager*)context)->monitor();
  }

  void* Manager::monitor() {
    MonitorCallback callback;

    int result = pthread_mutex_lock(&monitorMutex_);
    if (result != 0) {
      return NULL;
    }

    callback = monitorCallback_;

    result = pthread_mutex_unlock(&monitorMutex_);
    if (result != 0) {
      return NULL;
    }

    log("Monitoring...");
    do {
      ResponseFrame* response = serial_.receiveAny();
      if (response != NULL) {
	callback(response);
	delete response;
      }
    } while(true);
    
    return NULL;
  }
  
}

