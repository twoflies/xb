/***********************************************************/
/* frame                                                   */
/***********************************************************/

#include "frame.h"

#include <iostream>

#include "log.h"

namespace XB {

  const byte START_DELIMITER = ((byte)0x7E);

  FrameHeader::FrameHeader() {
  }

  int FrameHeader::read(int fd, long timeout) {
    byte data = 0;
    int result;
    while ((result = _fdread(fd, &data, 1, timeout)) == 0) {
      if (data == START_DELIMITER) {
	break;
      }
    }
    
    if (result < 0) {
      return result;
    }

    _log("[");

    byte length[2];
    result = fdread(fd, length, 2);
    if (result != 0) {
      return result;
    }
        
    length_ = (((unsigned short)length[0] << 8) & 0xFF00) | ((unsigned short)length[1] & 0x00FF);  
    if (length_ == 0) {
      return 0;
    }
    
    result = fdread(fd, &type_);
    if (result != 0) {
      return result;
    }

    _log(FrameHeader::getTypeCode(type_).c_str());
    
    result = fdread(fd, &id_);
    if (result != 0) {
      return result;
    }
    
    return 0;
  }

  unsigned short FrameHeader::getLength() {
    return length_;
  }

  unsigned short FrameHeader::getPayloadLength() {
    return length_ - 2;
  }

  byte FrameHeader::getType() {
    return type_;
  }

  byte FrameHeader::getId() {
    return id_;
  }

  byte Frame::start_ = START_DELIMITER;

  Frame::Frame(byte type, byte id) {
    type_ = type;
    id_ = id;
    checksum_ = 0;
  }

  Frame::Frame(FrameHeader* header) {
    type_ = header->getType();
    id_ = header->getId();
    checksum_ = 0;
  }
  
  Frame::~Frame() {
  }

  byte Frame::getType() {
    return type_;
  }

  byte Frame::getId() {
    return id_;
  }


  RequestFrame::RequestFrame(byte type, byte id) : Frame(type, id) {
  }

  RequestFrame::~RequestFrame() {
  }

  int RequestFrame::write(int fd) {
    checksum_ = (byte)0x00;
    
    int result = _fdwrite(fd, &start_);
    if (result != 0) {
      return result;
    }
    
    _log("[");
    
    unsigned short ilength = getPayloadLength() + 2;
    byte length[2];
    length[0] = (byte)((ilength >> 8) & 0xFF);
    length[1] = (byte)(ilength & 0xFF);
    result = fdwrite(fd, length, 2);
    if (result != 0) {
      return result;
    }
    
    result = writeAccumulate(fd, &type_);
    if (result != 0) {
      return result;
    }
    
    _log(FrameHeader::getTypeCode(type_).c_str());
    
    result = writeAccumulate(fd, &id_);
    if (result != 0) {
      return result;
    }

    result = writePayload(fd);
    if (result != 0) {
      return result;
    }
    
    byte checksum = (byte)0xFF - checksum_;
    result = fdwrite(fd, &checksum);
    if (result != 0) {
      return result;
    }
    
    log("]");
    
    return result;
  }
  
  int RequestFrame::writeAccumulate(int fd, byte* data, unsigned short length) {
    for (int index = 0; index < length; index++) {
      checksum_ += data[index];
    }
  
    return fdwrite(fd, data, length);
  }


  ResponseFrame::ResponseFrame(byte type) : Frame(type) {
  }
  
  ResponseFrame::ResponseFrame(FrameHeader* header) : Frame(header->getType(), header->getId()) {
    accumulate(&type_);
    accumulate(&id_);
  }

  ResponseFrame::~ResponseFrame() {
  }

  int ResponseFrame::read(int fd) {
    FrameHeader header;
    int result = readHeader(fd, &header);
    if (result != 0) {
      return result;
    }
    
    unsigned short payloadLength = header.getPayloadLength();
    result = readPayload(fd, payloadLength);
    if (result != 0) {
      return result;
    }
    
    return readAndValidateChecksum(fd);
  }

  int ResponseFrame::readHeader(int fd, FrameHeader* header) {
    int result = header->read(fd);
    if (result != 0) {
      return result;
    }
  
    type_ = header->getType();
    id_ = header->getId();
    
    return 0;
  }

  int ResponseFrame::readPayload(int fd, unsigned short length) {
    byte* data = new byte[length];
    int result = readAccumulate(fd, data, length);
    if (result != 0) {
      return result;
    }

    _log(" ");
    _logData(data, length);
    delete [] data;

    return 0;
  }
  
  int ResponseFrame::readAndValidateChecksum(int fd) {
    byte checksum;
    int result = fdread(fd, &checksum);
    if (result != 0) {
      return result;
    }

    if ((checksum + checksum_) != (byte)0xFF) {
      return ERROR_CHECKSUM;
    }

    _log("]");
    if (getStatus() != STATUS_OK) {
      log(" \033[1;31m %02X\033[0m", getStatus());
    }
    else {
      log(" \033[1;32m \u2713\033[0m");
    }

    return 0;
  }

  byte ResponseFrame::getStatus() {
    return STATUS_OK;
  }
  
  void ResponseFrame::accumulate(byte* data, unsigned short length) {
    for (int index = 0; index < length; index++) {
      checksum_ += data[index];
    }
  }

  int ResponseFrame::readAccumulate(int fd, byte* data, unsigned short length) {
    int result = fdread(fd, data, length);
    if (result < 0) {
      return result;
    }
    
    accumulate(data, length);
    
    return 0;
  }
  
  
  CommandFrame::CommandFrame(Command command, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
  }

  CommandFrame::CommandFrame(Command command, Parameter parameter, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    parameter_ = parameter;
  }

  CommandFrame::CommandFrame(byte type, Command command, byte id) : RequestFrame(type, id) {
    command_ = command;
  }

  CommandFrame::CommandFrame(byte type, Command command, Parameter parameter, byte id) : RequestFrame(type, id) {
    command_ = command;
    parameter_ = parameter;
  }
  
  CommandFrame::~CommandFrame() {
  }

  unsigned short CommandFrame::getPayloadLength() {
    return sizeof(command_) + parameter_.length;
  }

  int CommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    _log(" %s", command_.std_string().c_str());
    
    if (parameter_.data != NULL) {
      result = writeAccumulate(fd, parameter_.data, parameter_.length);
      if (result != 0) {
	return result;
      }

      _log("=");
      _logData(parameter_.data, parameter_.length);
    }
    
    return 0;
  }


  CommandResponseFrame::CommandResponseFrame(byte type) : ResponseFrame(type) {
}

  CommandResponseFrame::CommandResponseFrame(FrameHeader* header) : ResponseFrame(header) {
  }

  CommandResponseFrame::~CommandResponseFrame() {
    if ((parameter_.data != NULL) && (parameter_.data != (byte*)&parameter_.value)) {
      delete [] parameter_.data;
    }
  }

  Command CommandResponseFrame::getCommand() {
    return command_;
  }

  byte CommandResponseFrame::getStatus() {
    return status_;
  }

  Parameter CommandResponseFrame::getParameter() {
    return parameter_;
  }

  Parameter CommandResponseFrame::detachParameter() {
    Parameter parameter = parameter_;
    parameter_.data = NULL;
    parameter_.length = 0;
    return parameter;
  }

  int CommandResponseFrame::readPayload(int fd, unsigned short length) {
    int result = readAccumulate(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    _log(" %s", command_.std_string().c_str());
    
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

      _log("=");
      _logData(parameter_.data, parameter_.length);
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

    _log(" @");
    _logData((byte*)&address64_, sizeof(address64_));
    
    result = writeAccumulate(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    _log(",@");
    _logData((byte*)&address16_, sizeof(address16_));
    
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

  Address64 RemoteCommandResponseFrame::getAddress64() {
    return address64_;
  }

  Address16 RemoteCommandResponseFrame::getAddress16() {
    return address16_;
  }

  int RemoteCommandResponseFrame::readPayload(int fd, unsigned short length) {
    int result = readAccumulate(fd, (byte*)&address64_, sizeof(address64_));
    if (result != 0) {
      return result;
    }

    _log(" @");
    _logData((byte*)&address64_, sizeof(address64_));
    
    result = readAccumulate(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    _log(",@");
    _logData((byte*)&address16_, sizeof(address16_));
    
    length -= sizeof(address64_) + sizeof(address16_);
    return CommandResponseFrame::readPayload(fd, length);
  }

  int _logData(unsigned char* data, unsigned short length) {
    int result = -1;
    for (int index = 0; index < length; index++) {
      if (_log("%s%02X", (index > 0) ? "-" : "", data[index]) != -1) {
	result = 0;
      }
    }

    return result;
  }
  
  int logData(unsigned char* data, unsigned short length) {
    int result = _logData(data, length);
    if (result != -1) {
      std::cout << std::endl;
    }

    return result;
  }
  
}
