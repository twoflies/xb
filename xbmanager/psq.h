/*********************************************************************/
/* psq                                                               */
/*********************************************************************/

#ifndef _PSQ_H_
#define _PSQ_H_

#include <queue>
#include <list>
#include <pthread.h>

namespace XB {

  const std::size_t DEFAULT_MAX_SIZE = 20;

  template<typename T>
    struct default_delete {
      virtual void _delete(T value) { }
    };

  template<typename T>
    struct default_delete<T*> {
    virtual void _delete(T* value) {
      delete value;
    }
  };

  template<typename T>
    struct default_delete<T[]> {
    virtual void _delete(T* value) {
      delete [] value;
    }
  };

  template<class T>
    class PubSubQueueSubscriber {
  public:
    virtual ~PubSubQueueSubscriber() {}
    virtual void received(T) = 0;
  };

  template<class T>
  class PubSubQueue {
  public:
    PubSubQueue(std::size_t maxSize = DEFAULT_MAX_SIZE, default_delete<T> _delete = default_delete<T>());
    ~PubSubQueue();
    int initialize();
    int destroy();
    int publish(T value);
    int subscribe(PubSubQueueSubscriber<T>* subscriber);
    int unsubscribe(PubSubQueueSubscriber<T>* subscriber);

  private:
    static void* monitor_(void* context);
    void* monitor();

  private:
    std::size_t maxSize_;
    default_delete<T> delete_;
    std::queue<T> queue_;
    pthread_t queueThread_;
    pthread_mutex_t queueMutex_;
    pthread_cond_t queueCond_;
    std::list<PubSubQueueSubscriber<T>*> subscribers_;
  };
}

#include "psq.t.h"

#endif // _PSQ_H_
