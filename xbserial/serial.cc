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

  int Serial::send(RequestFrame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }
  
    return frame->write(fd_);
  }

  int Serial::send(const RequestFrame& frame) {
    return send((RequestFrame*)&frame);
  }

  int Serial::receive(ResponseFrame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    return frame->read(fd_);
  }

  int Serial::receive(FrameHeader* header, ResponseFrame* frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    int result = frame->readPayload(fd_, header->getPayloadLength());
    if (result != 0) {
      return result;
    }

    return frame->readAndValidateChecksum(fd_);
  }

  byte Serial::getNextId() {
    return ++idSequence_;
  }

  ResponseFrame* Serial::receiveAny(byte id, long timeout) {
    FrameHeader header;
    while (header.read(fd_, timeout) >= 0) {
      byte type = header.getType();
      ResponseFrame *frame;
      switch (type) {
      case TYPE_COMMAND_RESPONSE:
	frame = new CommandResponseFrame(&header);
	break;
      case TYPE_REMOTE_COMMAND_RESPONSE:
	frame = new RemoteCommandResponseFrame(&header);
	break;
      default:
	frame = new ResponseFrame(&header);
	break;
      }

      int result = receive(&header, frame);
      if ((result != 0) && (id == 0)) {
	delete frame;
	return NULL;
      }

      if ((id == 0) || (id == header.getId())) {
        return frame;
      }

      delete frame;
    }

    return NULL;
  }

  ResponseFrame* Serial::sendForResponse(RequestFrame* frame) {
    if (frame->getId() == (byte)0x00) {
      return NULL;
    }

    int result = send(frame);
    if (result != 0) {
      return NULL;
    }

    return receiveAny(frame->getId());
  }

  ResponseFrame* Serial::sendForResponse(const RequestFrame& frame) {
    return sendForResponse((RequestFrame*)&frame);
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
    ResponseFrame* response = sendForResponse(CommandFrame(command, getNextId()));
    return dynamic_cast<CommandResponseFrame*>(response);
  }

  RemoteCommandResponseFrame* Serial::sendRemoteCommandForResponse(Address64 address64, Address16 address16, Command command) {
    ResponseFrame* response = sendForResponse(RemoteCommandFrame(address64, address16, 0, command, getNextId()));
    return dynamic_cast<RemoteCommandResponseFrame*>(response);
  }

  RemoteCommandResponseFrame* Serial::sendRemoteCommandForResponse(Module* module, Command command) {
    return sendRemoteCommandForResponse(module->address64, module->address16, command);
  }
  
  int Serial::getParameter(Command command, Parameter* parameter) {
    CommandResponseFrame* commandResponse = sendCommandForResponse(command);
    if (commandResponse == NULL) {
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }
    
    *parameter = commandResponse->detachParameter();
    byte status = commandResponse->getStatus();

    delete commandResponse;
    return status;
  }
  
  int Serial::getRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter *parameter) {
    RemoteCommandResponseFrame* remoteCommandResponse = sendRemoteCommandForResponse(address64, address16, command);
    if (remoteCommandResponse == NULL) {
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    *parameter = remoteCommandResponse->detachParameter();
    byte status = remoteCommandResponse->getStatus();

    delete remoteCommandResponse;
    return status;
  }

  int Serial::getRemoteParameter(Module* module, Command command, Parameter* parameter) {
    return getRemoteParameter(module->address64, module->address16, command, parameter);
  }
  
  int Serial::setParameter(Command command, Parameter parameter) {
    ResponseFrame* response = sendForResponse(CommandFrame(command, parameter, getNextId()));
    CommandResponseFrame* commandResponse = dynamic_cast<CommandResponseFrame*>(response);
    if (commandResponse == NULL) {
      if (response != NULL) {
	delete response;
      }
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    byte status = commandResponse->getStatus();

    delete commandResponse;
    return status;
  }
  
  int Serial::setRemoteParameter(Address64 address64, Address16 address16, Command command, Parameter parameter, byte options) {
    ResponseFrame* response = sendForResponse(RemoteCommandFrame(address64, address16, options, command, parameter, getNextId()));
    RemoteCommandResponseFrame* remoteCommandResponse = dynamic_cast<RemoteCommandResponseFrame*>(response);
    if (remoteCommandResponse == NULL) {
      if (response != NULL) {
	delete response;
      }
      return logError(-1, "No Response for %s", command.std_string().c_str());
    }

    byte status = remoteCommandResponse->getStatus();

    delete remoteCommandResponse;
    return status;
  }

  int Serial::setRemoteParameter(Module* module, Command command, Parameter parameter, byte options) {
    return setRemoteParameter(module->address64, module->address16, command, parameter, options);
  }
  
}
