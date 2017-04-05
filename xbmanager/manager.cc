/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#include "manager.h"

#include "../xbserial/log.h"

namespace XB {

  Manager::Manager() {
    serial_.open("/dev/ttyUSB0", 9600);
    
    pthread_mutex_init(&monitorMutex_, NULL);
  }
  
  Manager::~Manager() {
    serial_.close();
    
    pthread_mutex_destroy(&monitorMutex_);
  }

  int Manager::monitor(MonitorCallback callback) {
    int result = pthread_mutex_lock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    monitorCallback_ = callback;

    result = pthread_mutex_unlock(&monitorMutex_);
    if (result != 0) {
      return result;
    }

    return pthread_create(&monitorThread_, NULL, &Manager::monitor_, this);
  }
  
  int Manager::discover(DiscoverCallback callback) {
    Parameter parameter;
    int result = serial_.getParameter(XBCOMMAND("NT"), &parameter);
    if (result != 0) {
      return result;
    }

    int timeout = (int)parameter;
    log("%02X %02X", ((byte*)&timeout)[0], ((byte*)&timeout)[1]);
    log("%ud", (unsigned short)parameter);
    
    return 0;
  }

  void* Manager::monitor_(void *context) {
    return ((Manager*)context)->monitor();
  }

  void* Manager::monitor() { 
    return NULL;
  }
  
}

