/***********************************************************/
/* log                                                     */
/***********************************************************/

#ifndef _LOG_H_
#define _LOG_H_

#include <cstdarg>

namespace XB {

  int _log(const char* format, va_list args);

  int _log(const char* format, ...);

  void log(const char* format, va_list args);

  void log(const char* format, ...);

  int logError(int err, const char *format, ...);

}

#endif  // _LOG_H_
