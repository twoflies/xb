/*********************************************************************/
/* mr                                                                */
/*********************************************************************/

#ifndef _MR_H_
#define _MR_H_

#include <map>
#include <pthread.h>

namespace XB {

  template<typename K, typename T>
    class MessageRouter {
  public:
    MessageRouter();
    ~MessageRouter();
    int initialize();
    int destroy();
    int route(K key, T message);
    T waitForMessage(K key, unsigned short timeout = 0);

  private:
    std::map<K, T> map_;
    pthread_mutex_t mapMutex_;
    pthread_cond_t mapCond_;
  };
}

#include "mr.t.h"

#endif // _MR_H_
