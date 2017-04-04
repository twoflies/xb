/***********************************************************************/
/* Serial                                                              */
/***********************************************************************/
#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "frame.h"

namespace XB {

  const int ERROR_IDEV = -100;
  const int ERROR_IBAUD = -101;
  const int ERROR_NOPEN = -200;

  class Serial {
  public:
    Serial();
    Serial(const char *dev, int baud);
    int open(const char *dev, int baud);
    int close();
    int send(RequestFrame *frame);
    int receive(ResponseFrame *frame);
    int receive(FrameHeader *header, ResponseFrame *frame);
    ResponseFrame* receiveAny();

  private:
    int fd_;
  };

}

#endif // _SERIAL_H_
