/*********************************************************************/
/* psq                                                               */
/*********************************************************************/

#include <unistd.h>

#include "../xbserial/log.h"

namespace XB {

  template<class T>
  PubSubQueue<T>::PubSubQueue(std::size_t maxSize, default_delete<T> _delete) {
    maxSize_ = maxSize;
    delete_ = _delete;
  }

  template<class T>
  PubSubQueue<T>::~PubSubQueue() {
  }

  template<class T>
  int PubSubQueue<T>::initialize() {
    int result = pthread_mutex_init(&queueMutex_, NULL);
    if (result != 0) {
      return result;
    }

    result = pthread_cond_init(&queueCond_, NULL);
    if (result != 0) {
      return result;
    }

    return pthread_create(&queueThread_, NULL, &PubSubQueue<T>::monitor_, this);
  }

  template<class T>
  int PubSubQueue<T>::destroy() {
    int result = pthread_cancel(queueThread_);
    if (result != 0) {
      return result;
    }

    result = pthread_join(queueThread_, NULL);
    if (result != 0) {
      return result;
    }

    result = pthread_cond_destroy(&queueCond_);
    if (result != 0) {
      return result;
    }

    result = pthread_mutex_destroy(&queueMutex_);
    if (result != 0) {
      return result;
    }

    while (!queue_.empty()) {
      T current = queue_.front();
      delete_._delete(current);
      queue_.pop();
    }

    return 0;
  }

  template<class T>
  int PubSubQueue<T>::enqueue(T value) {
    int result = pthread_mutex_lock(&queueMutex_);
    if (result != 0) {
      return result;
    }

    queue_.push(value);

    pthread_cond_signal(&queueCond_);
    
    return pthread_mutex_unlock(&queueMutex_);
  }

  template<class T>
  int PubSubQueue<T>::subscribe(PubSubQueueCallback callback) {
    int result = pthread_mutex_lock(&queueMutex_);
    if (result != 0) {
      return result;
    }

    callbacks_.push_back(callback);

    return pthread_mutex_unlock(&queueMutex_);
  }

  template<class T>
  int PubSubQueue<T>::unsubscribe(PubSubQueueCallback callback) {
    int result = pthread_mutex_lock(&queueMutex_);
    if (result != 0) {
      return result;
    }

    callbacks_.remove(callback);

    return pthread_mutex_unlock(&queueMutex_);
  }

  template<class T>
  void* PubSubQueue<T>::monitor_(void* context) {
    return ((PubSubQueue<T>*)context)->monitor();
  }

  template<class T>
  void* PubSubQueue<T>::monitor() {
    while (true) {
      int result = pthread_mutex_lock(&queueMutex_);
      if (result != 0) {
	sleep(1);
	continue;
      }

      result = pthread_cond_wait(&queueCond_, &queueMutex_);
      if (result != 0) {
	continue;
      }

      std::queue<T> queueCopy(queue_);
      std::list<PubSubQueueCallback> callbacksCopy(callbacks_);
      while (!queue_.empty()) {
	queue_.pop();
      }

      result = pthread_mutex_unlock(&queueMutex_);
      if (result != 0) {
	continue;
      }

      while (!queueCopy.empty()) {
	T current = queueCopy.front();
	for (typename std::list<PubSubQueueCallback>::iterator it = callbacksCopy.begin(); it != callbacksCopy.end(); it++) {
	  (*it)(current);
	}
	queueCopy.pop();
      }
    }
    
    return NULL;
  }
}
