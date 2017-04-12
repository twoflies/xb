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
  class PubSubQueue {
  public:
    typedef void (*PubSubQueueCallback)(T);
    
    PubSubQueue(std::size_t maxSize = DEFAULT_MAX_SIZE, default_delete<T> _delete = default_delete<T>());
    ~PubSubQueue();
    int initialize();
    int destroy();
    int enqueue(T value);
    int subscribe(PubSubQueueCallback callback);
    int unsubscribe(PubSubQueueCallback callback);

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
    std::list<PubSubQueueCallback> callbacks_;
  };
}

#include "psq.t.cc"

#endif // _PSQ_H_
