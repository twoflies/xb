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
    Serial(const char* dev, int baud);
    int open(const char* dev, int baud);
    int close();
    int send(RequestFrame* frame);
    int receive(ResponseFrame* frame);
    ResponseFrame* receiveAny(byte id = 0);
    ResponseFrame* sendForResponse(RequestFrame* frame);
    
  public:
    int sendCommand(Command command);
    int sendRemoteCommand(Address64 address64, Address16 address16, Command command);
    int sendRemoteCommand(Module* module, Command command);
    CommandResponseFrame* sendCommandForResponse(Command command);
    RemoteCommandResponseFrame* sendRemoteCommandForResponse(Address64 address64, Address16 address16, Command command);
    RemoteCommandResponseFrame* sendRemoteCommandForResponse(Module* module, Command command);
    int getParameter(Command command, Parameter* parameter);
    int getRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter *parameter);
    int getRemoteParameter(Module* module, Command command, Parameter* parameter);
    int setParameter(Command command, Parameter parameter);
    int setRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter parameter, byte options = 0);
    int setRemoteParameter(Module* module, Command command, Parameter parameter, byte options = 0);
    
  private:
    int receive(FrameHeader* header, ResponseFrame* frame);
    byte getNextId();
    
  private:
    int fd_;
    byte idSequence_;
  };

}

#endif // _SERIAL_H_
