/*********************************************************************/
/* Manager                                                           */
/*********************************************************************/

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "serial.h"

#include <string.h>

namespace XB {

  typedef int (*ManagerCallback)(ResponseFrame* frame);

  struct Module {
    Address16 address16;
    Address64 address64;
    
    Module(Address16* address16, Address64* address64) {
      memcpy(&this->address16, address16, sizeof(this->address16));
      memcpy(&this->address64, address64, sizeof(this->address64));
    }
  };
  
  typedef int (*DiscoverCallback)(Module*);
  
  class Manager {
  public:
    Manager(ManagerCallback callback);
    ~Manager();
    int discover(DiscoverCallback callback);
  };
  
}

#endif  // _MANAGER_H_
