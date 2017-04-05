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

  int FrameHeader::read(int fd) {
    byte data = 0;
    int result;
    while ((result = _fdread(fd, &data)) == 0) {
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
        
    length_ = (((int)length[0] << 8) & 0xFF00) | ((int)length[1] & 0x00FF);  
    if (length_ <= 0) {  // STND: Can this actually happen?
      return (length_ == 0) ? 0 : -1;
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

  int FrameHeader::getLength() {
    return length_;
  }

  int FrameHeader::getPayloadLength() {
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
    
    int ilength = getPayloadLength() + 2;
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
  
  int RequestFrame::writeAccumulate(int fd, byte* data, int length) {
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
    
    int payloadLength = header.getPayloadLength();
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

  int ResponseFrame::readPayload(int fd, int length) {
    byte dummy;
    for (int index = 0; index < length; index++) {
      int result = readAccumulate(fd, &dummy);
      if (result != 0) {
	return result;
      }
    }

    return 0;
  }
  
  int ResponseFrame::readAndValidateChecksum(int fd) {
    byte checksum;
    int result = fdread(fd, &checksum);
    if (result != 0) {
      return result;
    }

    log("]");
    
    return (checksum + checksum_) == (byte)0xFF;
  }
  
  void ResponseFrame::accumulate(byte* data, int length) {
    for (int index = 0; index < length; index++) {
      checksum_ += data[index];
    }
  }

  int ResponseFrame::readAccumulate(int fd, byte* data, int length) {
    int result = fdread(fd, data, length);
    if (result < 0) {
      return result;
    }
    
    accumulate(data, length);
    
    return 0;
  }
  
  
  CommandFrame::CommandFrame(Command command, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    data_ = NULL;
    length_ = 0;
  }

  CommandFrame::CommandFrame(Command command, Parameter parameter, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    parameter_ = parameter;
    data_ = (byte*)&parameter_;
    length_ = sizeof(parameter_);
  }

  CommandFrame::CommandFrame(Command command, byte* data, int length, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    data_ = data;
    length_ = length;
  }

  CommandFrame::CommandFrame(byte type, Command command, byte id) : RequestFrame(type, id) {
    command_ = command;
    data_ = NULL;
    length_ = 0;
  }

  CommandFrame::CommandFrame(byte type, Command command, Parameter parameter, byte id) : RequestFrame(type, id) {
    command_ = command;
    parameter_ = parameter;
    data_ = (byte*)&parameter_;
    length_ = sizeof(parameter_);
  }
  
  CommandFrame::CommandFrame(byte type, Command command, byte* data, int length, byte id) : RequestFrame(type, id) {
    command_ = command;
    data_ = data;
    length_ = length;
  }
  
  CommandFrame::~CommandFrame() {
  }

  int CommandFrame::getPayloadLength() {
    return sizeof(command_) + length_;
  }

  int CommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    _log(" %s", XBCOMMANDSTR(command_).c_str());
    
    if (data_ != NULL) {
      result = writeAccumulate(fd, data_, length_);
      if (result != 0) {
	return result;
      }

      _log(" =");
      _logData(data_, length_);
    }
    
    return 0;
  }


  CommandResponseFrame::CommandResponseFrame(byte type) : ResponseFrame(type) {
}

  CommandResponseFrame::CommandResponseFrame(FrameHeader* header) : ResponseFrame(header) {
  }

  CommandResponseFrame::~CommandResponseFrame() {
    if (data_ != NULL) {
      delete [] data_;
    }
  }

  Command CommandResponseFrame::getCommand() {
    return command_;
  }

  byte CommandResponseFrame::getStatus() {
    return status_;
  }

  byte* CommandResponseFrame::getData() {
    return data_;
  }

  int CommandResponseFrame::getLength() {
    return length_;
  }

  int CommandResponseFrame::readPayload(int fd, int length) {
    int result = fdread(fd, (byte*)&command_, sizeof(command_));
    if (result != 0) {
      return result;
    }

    _log(" %s", XBCOMMANDSTR(command_).c_str());
    
    result = fdread(fd, &status_);
    if (result != 0) {
      return result;
    }
    
    length_ = length - sizeof(command_) - 1;
    if (length_ > 0) {
      data_ = new byte[length_];
      result = fdread(fd, data_, length_);
      if (result != 0) {
	return result;
      }

      _log(" =");
      _logData(data_, length_);
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

  RemoteCommandFrame::RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, byte* data, int length, byte id) : CommandFrame(TYPE_REMOTE_COMMAND, command, data, length, id) {
    address64_ = address64;
    address16_ = address16;
    options_ = options;
  }

  RemoteCommandFrame::~RemoteCommandFrame() {
  }

  int RemoteCommandFrame::getPayloadLength() {
    return sizeof(address64_) + sizeof(address16_) + sizeof(options_) + CommandFrame::getPayloadLength();
  }

  RemoteCommandFrame* RemoteCommandFrame::toCoordinator(Command command, byte id) {
    return new RemoteCommandFrame(XBCOORDINATOR, XBUNKNOWN, 0, command, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toCoordinator(Command command, Parameter parameter, byte id, byte options) {
    return new RemoteCommandFrame(XBCOORDINATOR, XBUNKNOWN, options, command, parameter, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toBroadcast(Command command, byte id) {
    return new RemoteCommandFrame(XBBROADCAST, XBUNKNOWN, 0, command, id);
  }

  RemoteCommandFrame* RemoteCommandFrame::toBroadcast(Command command, Parameter parameter, byte id, byte options) {
    return new RemoteCommandFrame(XBBROADCAST, XBUNKNOWN, options, command, parameter, id);
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

    _log(" @");
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

  int RemoteCommandResponseFrame::readPayload(int fd, int length) {
    int result = fdread(fd, (byte*)&address64_, sizeof(address64_));
    if (result != 0) {
      return result;
    }

    _log(" @");
    _logData((byte*)&address64_, sizeof(address64_));
    
    result = fdread(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    _log(" @");
    _logData((byte*)&address16_, sizeof(address16_));
    
    length -= sizeof(address64_) + sizeof(address16_);
    return CommandResponseFrame::readPayload(fd, length);
  }

  int _logData(unsigned char* data, int length) {
    int result = -1;
    for (int index = 0; index < length; index++) {
      if (_log("%s%02X", (index > 0) ? " " : "", data[index]) != -1) {
	result = 0;
      }
    }

    return result;
  }
  
  int logData(unsigned char* data, int length) {
    int result = _logData(data, length);
    if (result != -1) {
      std::cout << std::endl;
    }

    return result;
  }
  
}
