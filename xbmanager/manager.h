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

  typedef void (*IOSampleFrameCallback)(const IOSampleFrame* frame);

  typedef std::pair<Command, Parameter> CommandParameter;

  struct ModuleConfiguration {
    std::string identifier;
    std::vector<CommandParameter> commandParameters;
  };
  
  class Manager {
  public:
    Manager();
    ~Manager();
    int initialize();
    int destroy();
    int discoverModules(std::vector<Module*>& modules);
    int configureModule(Module* module, ModuleConfiguration* configuration);
    int setModuleIdentifier(Module* module, char* identifier);
    int subscribeIOSample(IOSampleFrameCallback callback);
    int unsubscribeIOSample(IOSampleFrameCallback callback);

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
