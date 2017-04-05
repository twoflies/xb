/***********************************************************/
/* log                                                     */
/***********************************************************/

#include "log.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

namespace XB {

  int _log(const char* format, va_list args) {
    char* message;
    int result = vasprintf(&message, format, args);
    if (result != -1) {
      std::cout << message;
      free(message);
    }
    return result;
  }

  int _log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = _log(format, args);
    va_end(args);

    return result;
  }

  void log(const char* format, va_list args) {
    if (_log(format, args) != -1) {
      std::cout << std::endl;
    }
  }

  void log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(format, args);
    va_end(args);
  }

  int logError(int err, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = _log(format, args);
    if (result != -1) {
      if (err == -1) {
	std::cout << " (" << strerror(errno) << ")";
      }
      std::cout << std::endl;
    }
    va_end(args);
    
    return err;
  }
  
}
