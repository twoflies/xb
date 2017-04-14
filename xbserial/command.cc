/***********************************************************/
/* command                                                 */
/***********************************************************/

#include "command.h"

#include "log.h"

namespace XB {

  CommandFrame::CommandFrame(Command command, byte id) : Frame(TYPE_COMMAND) {
    id_ = id;
    command_ = command;
  }

  CommandFrame::CommandFrame(Command command, Parameter parameter, byte id) : Frame(TYPE_COMMAND) {
    id_ = id;
    command_ = command;
    parameter_ = parameter;
  }

  CommandFrame::CommandFrame(byte type, Command command, byte id) : Frame(type) {
    id_ = id;
    command_ = command;
  }

  CommandFrame::CommandFrame(byte type, Command command, Parameter parameter, byte id) : Frame(type) {
    id_ = id;
    command_ = command;
    parameter_ = parameter;
  }
  
  CommandFrame::~CommandFrame() {
  }

  byte CommandFrame::getId() const {
    return id_;
  }

  unsigned short CommandFrame::getPayloadLength() {
    return sizeof(id_) + sizeof(command_) + parameter_.length + Frame::getPayloadLength();
  }

  int CommandFrame::writePayloadPrologue(int fd) {
    int result = Frame::writePayloadPrologue(fd);
    if (result != 0) {
      return result;
    }
    
    result = writeAccumulate(fd, &id_);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" #%02X", id_);
    }

    return 0;
  }

  int CommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" %s", command_.std_string().c_str());
    }
    
    if (parameter_.data != NULL) {
      result = writeAccumulate(fd, parameter_.data, parameter_.length);
      if (result != 0) {
	return result;
      }

      if (DEBUG_FRAMES) {
	_log("=");
	_logData(parameter_.data, parameter_.length);
      }
    }
    
    return Frame::writePayload(fd);
  }


  CommandResponseFrame::CommandResponseFrame(byte type) : Frame(type) {
}

  CommandResponseFrame::CommandResponseFrame(FrameHeader* header) : Frame(header) {
  }

  CommandResponseFrame::~CommandResponseFrame() {
    if ((parameter_.data != NULL) && (parameter_.data != (byte*)&parameter_.value)) {
      delete [] parameter_.data;
    }
  }

  byte CommandResponseFrame::getId() const {
    return id_;
  }

  Command CommandResponseFrame::getCommand() const {
    return command_;
  }

  byte CommandResponseFrame::getStatus() const {
    return status_;
  }

  Parameter CommandResponseFrame::getParameter() const {
    return parameter_;
  }

  Parameter CommandResponseFrame::detachParameter() {
    Parameter parameter = parameter_;
    parameter_.data = NULL;
    parameter_.length = 0;
    return parameter;
  }

  unsigned short CommandResponseFrame::getPayloadPrologueLength() {
    return sizeof(id_) + Frame::getPayloadPrologueLength();
  }

  int CommandResponseFrame::readPayloadPrologue(int fd) {
    int result = readAccumulate(fd, &id_);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" #%02X", id_);
    }

    return 0;
  }

  int CommandResponseFrame::readPayload(int fd, unsigned short length) {
    int result = readAccumulate(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" %s", command_.std_string().c_str());
    }
    
    result = readAccumulate(fd, &status_);
    if (result != 0) {
      return result;
    }
    
    parameter_.length = length - sizeof(command_) - 1;
    if (parameter_.length > 0) {
      parameter_.data = new byte[parameter_.length];
      result = readAccumulate(fd, parameter_.data, parameter_.length);
      if (result != 0) {
	return result;
      }

      if (DEBUG_FRAMES) {
	_log("=");
	_logData(parameter_.data, parameter_.length);
      }
    }
    
    return 0;
  }

  
  RemoteCommandFrame::RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, byte id) : CommandFrame(TYPE_REMOTE_COMMAND, command, id) {
    address64_ = address64;
    address16_ = address16;
    options_ = options;
  }

  RemoteCommandFrame::RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, Parameter parameter, byte id) : CommandFrame(TYPE_REMOTE_COMMAND, command, parameter, id) {
    address64_ = address64;
    address16_ = address16;
    options_ = options;
  }

  RemoteCommandFrame::~RemoteCommandFrame() {
  }

  unsigned short RemoteCommandFrame::getPayloadLength() {
    return sizeof(address64_) + sizeof(address16_) + sizeof(options_) + CommandFrame::getPayloadLength();
  }

  RemoteCommandFrame* RemoteCommandFrame::toCoordinator(Command command, byte id) {
    return new RemoteCommandFrame(COORDINATOR, UNKNOWN, 0, command, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toCoordinator(Command command, Parameter parameter, byte id, byte options) {
    return new RemoteCommandFrame(COORDINATOR, UNKNOWN, options, command, parameter, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toBroadcast(Command command, byte id) {
    return new RemoteCommandFrame(BROADCAST, UNKNOWN, 0, command, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toBroadcast(Command command, Parameter parameter, byte id, byte options) {
    return new RemoteCommandFrame(BROADCAST, UNKNOWN, options, command, parameter, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toModule(Module* module, Command command, byte id) {
    return new RemoteCommandFrame(module->address64, module->address16, 0, command, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toModule(Module* module, Command command, Parameter parameter, byte id, byte options) {
    return new RemoteCommandFrame(module->address64, module->address16, options, command, parameter, id);
  }
  
  int RemoteCommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)&address64_, sizeof(address64_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" @");
      _logData((byte*)&address64_, sizeof(address64_));
    }
    
    result = writeAccumulate(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(",@");
      _logData((byte*)&address16_, sizeof(address16_));
    }
    
    result = writeAccumulate(fd, &options_);
    if (result != 0) {
      return result;
    }

    return CommandFrame::writePayload(fd);
  }


  RemoteCommandResponseFrame::RemoteCommandResponseFrame(byte type) : CommandResponseFrame(type) {
  }

  RemoteCommandResponseFrame::RemoteCommandResponseFrame(FrameHeader* header) : CommandResponseFrame(header) {
  }

  RemoteCommandResponseFrame::~RemoteCommandResponseFrame() {
  }

  Address64 RemoteCommandResponseFrame::getAddress64() const {
    return address64_;
  }

  Address16 RemoteCommandResponseFrame::getAddress16() const {
    return address16_;
  }

  int RemoteCommandResponseFrame::readPayload(int fd, unsigned short length) {
    int result = readAccumulate(fd, (byte*)&address64_, sizeof(address64_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" @");
      _logData((byte*)&address64_, sizeof(address64_));
    }
    
    result = readAccumulate(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(",@");
      _logData((byte*)&address16_, sizeof(address16_));
    }
    
    length -= sizeof(address64_) + sizeof(address16_);
    return CommandResponseFrame::readPayload(fd, length);
  }

}
