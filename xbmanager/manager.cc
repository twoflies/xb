/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#include "manager.h"

#include <sstream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../xbserial/log.h"

namespace XB {

  Manager::Manager() {
    idSequence_ = 0;
  }
  
  Manager::~Manager() {
  }

  int Manager::initialize() {
    int result = serial_.open("/dev/ttyUSB0", 9600);
    if (result != 0) {
      return result;
    }

    result = commandResponseRouter_.initialize();
    if (result != 0) {
      return result;
    }

    result = ioSampleQueue_.initialize();
    if (result != 0) {
      return result;
    }

    result = pthread_create(&monitorThread_, NULL, &Manager::monitor_, this);
    if (result != 0) {
      return result;
    }

    return 0;
  }

  int Manager::destroy() {
    int result = pthread_cancel(monitorThread_);
    if (result != 0) {
      return result;
    }

    result = pthread_join(monitorThread_, NULL);
    if (result != 0) {
      return result;
    }

    result = commandResponseRouter_.destroy();
    if (result != 0) {
      return result;
    }

    result = ioSampleQueue_.destroy();
    if (result != 0) {
      return result;
    }

    return serial_.close();
  }

  int Manager::discoverModules(std::vector<Module*>& modules) {
    Parameter parameter;
    int result = getParameter(Command("NT"), &parameter);
    if (result != 0) {
      return result;
    }

    byte id = getNextId();
    result = serial_.send(CommandFrame(Command("ND"), id));
    if (result != 0) {
      return result;
    }

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    short timeRemaining = parameter.ushort() * 100;
    while (timeRemaining > 0) {
      CommandResponseFrame* response = commandResponseRouter_.waitForMessage(id, timeRemaining);
      if (response != NULL) {
	byte* data = response->getParameter().data;
	int length = response->getParameter().length;

	if (length > (int)(sizeof(Address16) + sizeof(Address64))) {
	  Module* module = new Module();
	  module->address16 = Address16(data);
	  module->address64 = Address64(data + sizeof(module->address16));
	  module->identifier = std::string(reinterpret_cast<const char*>(data + sizeof(module->address16) + sizeof(module->address64)));

	  modules.push_back(module);
	}

	delete response;
      }
     
      struct timespec current;
      clock_gettime(CLOCK_MONOTONIC, &current);
      unsigned long long diff = (1000000000 * (current.tv_sec - start.tv_sec) + current.tv_nsec - start.tv_nsec) / 1000000;
      timeRemaining -= diff;
    }
    
    return 0;
  }

  int Manager::configureModule(Module* module, ModuleConfiguration* configuration) {
    module->identifier = configuration->identifier;
    if (module->identifier.empty()) {
      std::ostringstream identifier;
      identifier << "Node " << std::hex << std::uppercase << (int)module->address16.a << "-" << (int)module->address16.b;
      module->identifier = identifier.str();
    }

    int result = setModuleIdentifier(module, module->identifier.c_str());
    if (result != 0) {
      return result;
    }

    for (std::vector<CommandParameter*>::iterator it = configuration->commandParameters.begin(); it != configuration->commandParameters.end(); it++) {
      result = setRemoteParameter(module, (*it)->command, (*it)->parameter);
      if (result != 0) {
	return result;
      }
    }

    if (!configuration->commandParameters.empty()) {
      CommandResponseFrame* response = sendRemoteCommandForResponse(module, Command("AC"));
      if (response == NULL) {
	result = -1;
      }
    }

    return result;
  }

  int Manager::setModuleIdentifier(Module* module, const char* identifier) {
    return setRemoteParameter(module, Command("NI"), Parameter(identifier), OPTION_APPLY);
  }

  int Manager::subscribeIOSample(IOSampleFrameSubscriber* subscriber) {
    return ioSampleQueue_.subscribe(subscriber);
  }

  int Manager::unsubscribeIOSample(IOSampleFrameSubscriber* subscriber) {
    return ioSampleQueue_.unsubscribe(subscriber);
  }

  CommandResponseFrame* Manager::sendCommandForResponse(Command command, Parameter parameter) {
    return sendCommandForResponse(CommandFrame(command, parameter, getNextId()));
  }
  
  RemoteCommandResponseFrame* Manager::sendRemoteCommandForResponse(Module* module, Command command, Parameter parameter, byte options) {
    CommandResponseFrame* response = sendCommandForResponse(RemoteCommandFrame(module->address64, module->address16, options, command, parameter, getNextId()));
    RemoteCommandResponseFrame *remoteResponse = dynamic_cast<RemoteCommandResponseFrame*>(response);
    if ((remoteResponse == NULL) && (response != NULL)) {
      delete response;
    }

    return remoteResponse;
  }
  
  int Manager::getParameter(Command command, Parameter* parameter) {
    CommandResponseFrame* response = sendCommandForResponse(command);
    if (response == NULL) {
      return -1;
    }

    *parameter = response->detachParameter();
    byte status = response->getStatus();

    delete response;
    return status;
  }
  
  int Manager::getRemoteParameter(Module* module, Command command, Parameter* parameter) {
    RemoteCommandResponseFrame* response = sendRemoteCommandForResponse(module, command);
    if (response == NULL) {
      return -1;
    }

    *parameter = response->detachParameter();
    byte status = response->getStatus();

    delete response;
    return status;
  }
  
  int Manager::setParameter(Command command, Parameter parameter) {
    CommandResponseFrame* response = sendCommandForResponse(command, parameter);
    if (response == NULL) {
      return -1;
    }

    byte status = response->getStatus();

    delete response;
    return status;
  }
  
  int Manager::setRemoteParameter(Module* module, Command command, Parameter parameter, byte options) {
    RemoteCommandResponseFrame* response = sendRemoteCommandForResponse(module, command, parameter, options);
    if (response == NULL) {
      return -1;
    }

    byte status = response->getStatus();

    delete response;
    return status;
  }
  

  byte Manager::getNextId() {
    return ++idSequence_;
  }

  CommandResponseFrame* Manager::sendCommandForResponse(const CommandFrame& frame) {
    if (frame.getId() == 0) {
      return NULL;
    }
    
    int result = serial_.send(frame);
    if (result != 0) {
      return NULL;
    }

    return commandResponseRouter_.waitForMessage(frame.getId());
  }

  void* Manager::monitor_(void *context) {
    return ((Manager*)context)->monitor();
  }

  void* Manager::monitor() {
    do {
      Frame* response = serial_.receiveAny();

      CommandResponseFrame* commandResponse = dynamic_cast<CommandResponseFrame*>(response);
      if (commandResponse != NULL) {
	commandResponseRouter_.route(commandResponse->getId(), commandResponse);
	continue;
      }

      IOSampleFrame* ioSample = dynamic_cast<IOSampleFrame*>(response);
      if (ioSample != NULL) {
	ioSampleQueue_.publish(ioSample);
	continue;
      }
      
      if (response != NULL) {
	delete response;
      }
    } while(true);
    
    return NULL;
  }
}

