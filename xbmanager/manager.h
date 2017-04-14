/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <string>
#include <vector>
#include <queue>
#include <pthread.h>

#include "../xbserial/serial.h"
#include "psq.h"
#include "mr.h"

namespace XB {

  typedef PubSubQueueSubscriber<const IOSampleFrame*> IOSampleFrameSubscriber;

  struct CommandParameter {
    const Command command;
    const Parameter parameter;

  CommandParameter(const char* command, unsigned short parameter) : command(command), parameter(parameter) {
    }
  };

  struct ModuleConfiguration {
    std::string identifier;
    std::vector<CommandParameter*> commandParameters;

    ~ModuleConfiguration() {
      for (std::vector<CommandParameter*>::iterator it = commandParameters.begin(); it != commandParameters.end(); it++) {
	delete *it;
      }
    }

    void addCommandParameter(const char* command, unsigned short parameter) {
      commandParameters.push_back(new CommandParameter(command, parameter));
    }
  };
  
  class Manager {
  public:
    Manager();
    ~Manager();
    int initialize();
    int destroy();
    int discoverModules(std::vector<Module*>& modules);
    int configureModule(Module* module, ModuleConfiguration* configuration);
    int setModuleIdentifier(Module* module, const char* identifier);
    int subscribeIOSample(IOSampleFrameSubscriber* subscriber);
    int unsubscribeIOSample(IOSampleFrameSubscriber* subscriber);

  public:
    CommandResponseFrame* sendCommandForResponse(Command command, Parameter parameter = Parameter());
    RemoteCommandResponseFrame* sendRemoteCommandForResponse(Module* module, Command command, Parameter parameter = Parameter(), byte options = 0);
    int getParameter(Command command, Parameter* parameter);
    int getRemoteParameter(Module* module, Command command, Parameter* parameter);
    int setParameter(Command command, Parameter parameter);
    int setRemoteParameter(Module* module, Command command, Parameter parameter, byte options = 0);

  private:
    byte getNextId();
    CommandResponseFrame* sendCommandForResponse(const CommandFrame& frame);

  private:
    static void* monitor_(void* context);
    void* monitor();

  private:
    Serial serial_;
    pthread_t monitorThread_;
    MessageRouter<byte, CommandResponseFrame*> commandResponseRouter_;
    PubSubQueue<const IOSampleFrame*> ioSampleQueue_;
    byte idSequence_;
  };
  
}

#endif  // _MANAGER_H_
