/*********************************************************************/
/* mr                                                                */
/*********************************************************************/

#include <sys/time.h>

namespace XB {

  template<typename K, typename T>
  MessageRouter<K, T>::MessageRouter() {
  }

  template<typename K, typename T>
  MessageRouter<K, T>::~MessageRouter() {
  }

  template<typename K, typename T>
  int MessageRouter<K, T>::initialize() {
    int result = pthread_mutex_init(&mapMutex_, NULL);
    if (result != 0) {
      return result;
    }

    return pthread_cond_init(&mapCond_, NULL);
  }

  template<typename K, typename T>
  int MessageRouter<K, T>::destroy() {
    int result = pthread_mutex_destroy(&mapMutex_);
    if (result != 0) {
      return result;
    }

    return pthread_cond_destroy(&mapCond_);
  }

  template<typename K, typename T>
  int MessageRouter<K, T>::route(K key, T message) {
    int result = pthread_mutex_lock(&mapMutex_);
    if (result != 0) {
      return result;
    }

    map_[key] = message;

    pthread_cond_broadcast(&mapCond_);

    return pthread_mutex_unlock(&mapMutex_);
  }

  template<typename K, typename T>
  T MessageRouter<K, T>::waitForMessage(K key, unsigned short timeout) {
    int result = pthread_mutex_lock(&mapMutex_);
    if (result != 0) {
      return NULL;
    }

    if (timeout > 0) {
      timeval now;
      gettimeofday(&now, NULL);
      
      timespec ts;
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      ts.tv_nsec = (now.tv_usec * 1000) + ((timeout % 1000) * 1000000);
      pthread_cond_timedwait(&mapCond_, &mapMutex_, &ts);
    }
    else {
      pthread_cond_wait(&mapCond_, &mapMutex_);
    }

    T value = NULL;
    typename std::map<K, T>::iterator it = map_.find(key);
    if (it != map_.end()) {
      value = it->second;
      map_.erase(it);
    }

    pthread_mutex_unlock(&mapMutex_);

    return value;
  }
}
