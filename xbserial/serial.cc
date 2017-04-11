/***********************************************************************/
/* Serial                                                              */
/***********************************************************************/

#include "serial.h"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "file.h"
#include "log.h"

namespace XB {
  
  Serial::Serial() {
    fd_ = -1;
    idSequence_ = 0;
  }

  Serial::Serial(const char* dev, int baud) {
    fd_ = -1;
    idSequence_ = 0;
    open(dev, baud);
  }

  int Serial::open(const char* dev, int baud) {
    fd_ = ::open(dev, O_RDWR | O_NOCTTY /*| O_NDELAY*/);
    if (fd_ < 0) {
      return fd_;
    }

    if (!isatty(fd_)) {
      return ERROR_IDEV;
    }

    struct termios config;
    if (tcgetattr(fd_, &config) < 0) {
      return ERROR_IDEV;
    }

    config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
    config.c_oflag = 0;
    config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    config.c_cflag &= ~(CSIZE | PARENB);
    config.c_cflag |= CS8;

    config.c_cc[VMIN] = 1;
    config.c_cc[VTIME] = 0;

    if ((cfsetispeed(&config, B9600) < 0) || (cfsetospeed(&config, B9600) < 0)) {
      return ERROR_IBAUD;
    }

    if (tcsetattr(fd_, TCSAFLUSH, &config) < 0) {
      return ERROR_IDEV;
    }
    
    return 0;
  }

  int Serial::close() {
    return ::close(fd_);
  }

  int Serial::send(Frame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }
  
    return frame->write(fd_);
  }

  int Serial::send(const Frame& frame) {
    return send((Frame*)&frame);
  }

  int Serial::receive(Frame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    return frame->read(fd_);
  }

  int Serial::receiveFromHeader(FrameHeader* header, Frame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    return frame->readFromHeader(fd_, header);
  }

  byte Serial::getNextId() {
    return ++idSequence_;
  }

  Frame* Serial::receiveAny(long timeout) {
    FrameHeader header;
    while (header.read(fd_, timeout) >= 0) {
      byte type = header.getType();
      Frame *frame;
      switch (type) {
      case TYPE_COMMAND_RESPONSE:
	frame = new CommandResponseFrame(&header);
	break;
      case TYPE_REMOTE_COMMAND_RESPONSE:
	frame = new RemoteCommandResponseFrame(&header);
	break;
      default:
	frame = new Frame(&header);
	break;
      }

      int result = receiveFromHeader(&header, frame);
      if (result != 0) {
	delete frame;
	return NULL;
      }

      return frame;
    }

    return NULL;
  }

  CommandResponseFrame* Serial::receiveCommandResponse(byte id, long timeout) {
    FrameHeader header;
    while (header.read(fd_, timeout) >= 0) {
      byte type = header.getType();
      CommandResponseFrame *frame;
      switch (type) {
      case TYPE_COMMAND_RESPONSE:
	frame = new CommandResponseFrame(&header);
	break;
      case TYPE_REMOTE_COMMAND_RESPONSE:
	frame = new RemoteCommandResponseFrame(&header);
	break;
      default:
        continue;
      }

      int result = receiveFromHeader(&header, frame);
      if (result != 0) {
	delete frame;
	return NULL;
      }

      if (id == frame->getId()) {
        return frame;
      }

      delete frame;
    }

    return NULL;
  }

  CommandResponseFrame* Serial::sendCommandForResponse(CommandFrame* frame) {
    if (frame->getId() == (byte)0x00) {
      return NULL;
    }

    int result = send(frame);
    if (result != 0) {
      return NULL;
    }

    return receiveCommandResponse(frame->getId());
  }

  CommandResponseFrame* Serial::sendCommandForResponse(const CommandFrame& frame) {
    return sendCommandForResponse((CommandFrame*)&frame);
  }

  
  int Serial::sendCommand(Command command) {
    return send(CommandFrame(command, 0));
  }
  
  int Serial::sendRemoteCommand(Address64 address64, Address16 address16, Command command) {
    return send(RemoteCommandFrame(address64, address16, 0, command, 0));
  }
  
  int Serial::sendRemoteCommand(Module* module, Command command) {
    return sendRemoteCommand(module->address64, module->address16, command);
  }

  CommandResponseFrame* Serial::sendCommandForResponse(Command command) {
    return sendCommandForResponse(CommandFrame(command, getNextId()));
  }

  RemoteCommandResponseFrame* Serial::sendRemoteCommandForResponse(Address64 address64, Address16 address16, Command command) {
    CommandResponseFrame* response = sendCommandForResponse(RemoteCommandFrame(address64, address16, 0, command, getNextId()));
    return dynamic_cast<RemoteCommandResponseFrame*>(response);
  }

  RemoteCommandResponseFrame* Serial::sendRemoteCommandForResponse(Module* module, Command command) {
    return sendRemoteCommandForResponse(module->address64, module->address16, command);
  }
  
  int Serial::getParameter(Command command, Parameter* parameter) {
    CommandResponseFrame* response = sendCommandForResponse(command);
    if (response == NULL) {
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }
    
    *parameter = response->detachParameter();
    byte status = response->getStatus();

    delete response;
    return status;
  }
  
  int Serial::getRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter *parameter) {
    RemoteCommandResponseFrame* response = sendRemoteCommandForResponse(address64, address16, command);
    if (response == NULL) {
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    *parameter = response->detachParameter();
    byte status = response->getStatus();

    delete response;
    return status;
  }

  int Serial::getRemoteParameter(Module* module, Command command, Parameter* parameter) {
    return getRemoteParameter(module->address64, module->address16, command, parameter);
  }
  
  int Serial::setParameter(Command command, Parameter parameter) {
    CommandResponseFrame* response = sendCommandForResponse(CommandFrame(command, parameter, getNextId()));
    if (response == NULL) {
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    byte status = response->getStatus();

    delete response;
    return status;
  }
  
  int Serial::setRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter parameter, byte options) {
    CommandResponseFrame* response = sendCommandForResponse(RemoteCommandFrame(address64, address16, options, command, parameter, getNextId()));
    RemoteCommandResponseFrame* remoteResponse = dynamic_cast<RemoteCommandResponseFrame*>(response);
    if (remoteResponse == NULL) {
      if (response != NULL) {
	delete response;
      }
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    byte status = remoteResponse->getStatus();

    delete remoteResponse;
    return status;
  }

  int Serial::setRemoteParameter(Module* module, Command command, Parameter parameter, byte options) {
    return setRemoteParameter(module->address64, module->address16, command, parameter, options);
  }
  
}
