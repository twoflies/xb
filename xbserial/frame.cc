/***********************************************************/
/* frame                                                   */
/***********************************************************/

#include "frame.h"

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

void wdir(byte *data, int length = 1) {
  char buffer[3];
  for (int index = 0; index < length; index++) {
    snprintf(buffer, 3, "%02X", data[index]);
    std::cout << buffer << " ";
  }
  std::cout << std::endl;
}

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
    
    wdir(&data);
  
    if (result < 0) {
      return result;
    }

    byte length[2];
    result = fdread(fd, length, 2);
    if (result != 0) {
      return result;
    }
    
    wdir(length, 2);
    
    length_ = (((int)length[0] << 8) & 0xFF00) | ((int)length[1] & 0x00FF);  
    if (length_ <= 0) {  // STND: Can this actually happen?
      return (length_ == 0) ? 0 : -1;
    }
    
    result = fdread(fd, &type_);
    if (result != 0) {
      return result;
    }
    
    wdir(&type_);
    
    result = fdread(fd, &id_);
    if (result != 0) {
      return result;
    }
    
    wdir(&id_);
    
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

  Frame::Frame(FrameHeader *header) {
    type_ = header->getType();
    id_ = header->getId();
    checksum_ = 0;
  }
  
  Frame::~Frame() {
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
    
    wdir(&start_);
    
    int ilength = getPayloadLength() + 2;
    byte length[2];
    length[0] = (byte)((ilength >> 8) & 0xFF);
    length[1] = (byte)(ilength & 0xFF);
    result = fdwrite(fd, length, 2);
    if (result != 0) {
      return result;
    }
    
    wdir(length, 2);
    
    result = writeAccumulate(fd, &type_);
    if (result != 0) {
      return result;
    }
    
    wdir(&type_);
    
    result = writeAccumulate(fd, &id_);
    if (result != 0) {
      return result;
    }
    
    wdir(&id_);

    result = writePayload(fd);
    if (result != 0) {
      return result;
    }
    
    byte checksum = (byte)0xFF - checksum_;
    result = fdwrite(fd, &checksum);
    if (result != 0) {
      return result;
    }
    
    wdir(&checksum);
    
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
  
  ResponseFrame::ResponseFrame(byte type, byte id) : Frame(type, id) {
    accumulate(&type);
    accumulate(&id);
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
  
  int ResponseFrame::readAndValidateChecksum(int fd) {
    byte checksum;
    int result = fdread(fd, &checksum);
    if (result != 0) {
      return result;
    }
    
    wdir(&checksum);
    
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
  
  
  CommandFrame::CommandFrame(Command* command, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    parameter_ = NULL;
    length_ = 0;
  }

  CommandFrame::CommandFrame(Command* command, byte* parameter, int length, byte id) : RequestFrame(TYPE_COMMAND, id) {
    command_ = command;
    parameter_ = parameter;
    length_ = length;
  }
  
  CommandFrame::CommandFrame(byte type, Command* command, byte* parameter, int length, byte id) : RequestFrame(type, id) {
    command_ = command;
    parameter_ = parameter;
    length_ = length;
  }
  
  CommandFrame::~CommandFrame() {
  }

  int CommandFrame::getPayloadLength() {
    return sizeof(*command_) +
      (length_ * sizeof(*parameter_));
  }

  int CommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)command_, sizeof(*command_));
    if (result != 0) {
      return result;
    }

    wdir((byte*)command_, sizeof(*command_));
    
    if (parameter_ != NULL) {
      result = writeAccumulate(fd, parameter_, length_);
      if (result != 0) {
	return result;
      }
      
      wdir(parameter_, length_);
    }
    
    return 0;
  }


  CommandResponseFrame::CommandResponseFrame(byte type) : ResponseFrame(type) {
}

  CommandResponseFrame::CommandResponseFrame(FrameHeader* header) : ResponseFrame(header->getType(), header->getId()) {
  }

  CommandResponseFrame::~CommandResponseFrame() {
    if (command_ != NULL) {
      delete command_;
    }
  }

  Command* CommandResponseFrame::getCommand() {
    return command_;
  }

  byte* CommandResponseFrame::getParameter() {
    return parameter_;
  }

  int CommandResponseFrame::getLength() {
    return length_;
  }

  int CommandResponseFrame::readPayload(int fd, int length) {
    command_ = new Command;
    int result = fdread(fd, (byte*)command_, sizeof(*command_));
    if (result != 0) {
      return result;
    }

    wdir((byte*)command_, sizeof(*command_));
    
    result = fdread(fd, &status_);
    if (result != 0) {
      return result;
    }
    
    wdir(&status_);
    
    length_ = length - sizeof(*command_) - 1;
    if (length_ > 0) {
      parameter_ = new byte[length_];
      result = fdread(fd, parameter_, length_);
      if (result != 0) {
	return result;
      }
      
      wdir(parameter_, length_);
    }
    
    return 0;
  }


  Address64* Address64::COORDINATOR = new Address64(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  Address64* Address64::BROADCAST = new Address64(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF);
  
  Address16* Address16::UNKNOWN = new Address16(0xFF, 0xFE);
  
  RemoteCommandFrame::RemoteCommandFrame(Address64* address64, Address16* address16, byte options, Command* command, byte id) : CommandFrame(TYPE_REMOTE_COMMAND, command, NULL, 0, id) {
    address64_ = address64;
    address16_ = address16;
    options_ = options;
  }

  RemoteCommandFrame::RemoteCommandFrame(Address64* address64, Address16* address16, byte options, Command* command, byte* parameter, int length, byte id) : CommandFrame(TYPE_REMOTE_COMMAND, command, parameter, length, id) {
    address64_ = address64;
    address16_ = address16;
    options_ = options;
  }

  RemoteCommandFrame::~RemoteCommandFrame() {
  }

  int RemoteCommandFrame::getPayloadLength() {
    return sizeof(*address64_) + sizeof(*address16_) + sizeof(options_) + CommandFrame::getPayloadLength();
  }
  
  int RemoteCommandFrame::writePayload(int fd) {
    int result = writeAccumulate(fd, (byte*)address64_, sizeof(*address64_));
    if (result != 0) {
      return result;
    }

    wdir((byte*)address64_, sizeof(*address64_));
    
    result = writeAccumulate(fd, (byte*)address16_, sizeof(*address16_));
    if (result != 0) {
      return result;
    }
    
    wdir((byte*)address16_, sizeof(*address16_));
    
    result = writeAccumulate(fd, &options_);
    if (result != 0) {
      return result;
    }
    
    wdir(&options_);
    
    return CommandFrame::writePayload(fd);
  }


  RemoteCommandResponseFrame::RemoteCommandResponseFrame(byte type) : CommandResponseFrame(type) {
  }

  RemoteCommandResponseFrame::RemoteCommandResponseFrame(FrameHeader* header) : CommandResponseFrame(header) {
  }

  RemoteCommandResponseFrame::~RemoteCommandResponseFrame() {
    if (address64_ != NULL) {
      delete address64_;
    }
    
    if (address16_ != NULL) {
      delete address16_;
    }
  }

  Address64* RemoteCommandResponseFrame::getAddress64() {
    return address64_;
  }

  Address16* RemoteCommandResponseFrame::getAddress16() {
    return address16_;
  }

  int RemoteCommandResponseFrame::readPayload(int fd, int length) {
    address64_ = new Address64;
    int result = fdread(fd, (byte*)address64_, sizeof(*address64_));
    if (result != 0) {
      return result;
    }

    wdir((byte*)address64_, sizeof(*address64_));
    
    address16_ = new Address16;
    result = fdread(fd, (byte*)address16_, sizeof(*address16_));
    if (result != 0) {
      return result;
    }
    
    wdir((byte*)address16_, sizeof(*address16_));
    
    length -= sizeof(*address64_) + sizeof(*address16_);
    return CommandResponseFrame::readPayload(fd, length);
  }

}
