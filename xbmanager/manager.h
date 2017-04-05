/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "../xbserial/serial.h"

#include <pthread.h>

namespace XB {

  typedef int (*MonitorCallback)(ResponseFrame* frame);
  
  typedef int (*DiscoverCallback)(Module*);

  class Manager {
  public:
    Manager();
    ~Manager();
    int monitor(MonitorCallback callback);
    int discover(DiscoverCallback callback);
  private:
    static void* monitor_(void* context);
    void* monitor();

  private:
    Serial serial_;
    pthread_t monitorThread_;
    pthread_mutex_t monitorMutex_;
    MonitorCallback monitorCallback_;
    DiscoverCallback discoverCallback_;
  };
  
}

#endif  // _MANAGER_H_
