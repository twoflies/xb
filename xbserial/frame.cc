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

    if (DEBUG_FRAMES) {
      _log("[");
    }

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

    if (DEBUG_FRAMES) {
      _log(FrameHeader::getTypeCode(type_).c_str());
    }
    
    return 0;
  }

  unsigned short FrameHeader::getLength() {
    return length_;
  }

  byte FrameHeader::getType() {
    return type_;
  }

  
  byte Frame::start_ = START_DELIMITER;

  Frame::Frame(byte type) {
    type_ = type;
    checksum_ = 0;
  }

  Frame::Frame(FrameHeader* header) {
    type_ = header->getType();
    checksum_ = 0;

    accumulate(&type_);
  }
  
  Frame::~Frame() {
  }

  byte Frame::getType() {
    return type_;
  }

  int Frame::write(int fd) {
    int result = writeHeader(fd);
    if (result != 0) {
      return result;
    }

    result = writePayloadPrologue(fd);
    if (result != 0) {
      return result;
    }

    result = writePayload(fd);
    if (result != 0) {
      return result;
    }

    return writeChecksum(fd);
  }

  int Frame::read(int fd) {
    FrameHeader header;
    int result = readHeader(fd, &header);
    if (result != 0) {
      return result;
    }

    return readFromHeader(fd, &header);
  }

  int Frame::readFromHeader(int fd, FrameHeader* header) {
    int result = readPayloadPrologue(fd);
    if (result != 0) {
      return result;
    }
    
    int length = header->getLength() - getPayloadPrologueLength();
    result = readPayload(fd, length);
    if (result != 0) {
      return result;
    }
    
    return readChecksum(fd);
  }

  byte Frame::getStatus() {
    return STATUS_OK;
  }

  int Frame::writeHeader(int fd) {
    int result = _fdwrite(fd, &start_);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log("[");
    }
    
    unsigned short ilength = getPayloadLength();
    byte length[2];
    length[0] = (byte)((ilength >> 8) & 0xFF);
    length[1] = (byte)(ilength & 0xFF);
    result = fdwrite(fd, length, 2);
    if (result != 0) {
      return result;
    }
     
    return 0;
  }

  unsigned short Frame::getPayloadPrologueLength() {
    return sizeof(type_);
  }
  
  unsigned short Frame::getPayloadLength() {
    return sizeof(type_);
  }

  int Frame::writePayloadPrologue(int fd) {
    int result = writeAccumulate(fd, &type_);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(FrameHeader::getTypeCode(type_).c_str());
    }
      
    return 0;
  }
  
  int Frame::writePayload(int fd) {
    return 0;
  }
  
  int Frame::writeChecksum(int fd) {
    byte checksum = (byte)0xFF - checksum_;
    int result = fdwrite(fd, &checksum);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      log(" !%02X]", checksum);
    }
    
    return 0;
  }
  
  int Frame::readHeader(int fd, FrameHeader* header) {
    int result = header->read(fd);
    if (result != 0) {
      return result;
    }
    
    type_ = header->getType();
    accumulate(&type_);
    
    return 0;
  }

  int Frame::readPayloadPrologue(int fd) {
    // type_ read by FrameHeader
    return 0;
  }
  
  int Frame::readPayload(int fd, unsigned short length) {
    byte* data = new byte[length];
    int result = readAccumulate(fd, data, length);
    if (result != 0) {
      delete [] data;
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" ");
      _logData(data, length);
    }
    
    delete [] data;
    return 0;
  }
  
  int Frame::readChecksum(int fd) {
    byte checksum;
    int result = fdread(fd, &checksum);
    if (result != 0) {
      return result;
    }

    if ((checksum + checksum_) != (byte)0xFF) {
      log("Checksum error");
      return ERROR_CHECKSUM;
    }

    if (DEBUG_FRAMES) {
      _log(" !%02X]", checksum);
      if (getStatus() != STATUS_OK) {
	log(" \033[1;31m %02X\033[0m", getStatus());
      }
      else {
	log(" \033[1;32m \u2713\033[0m");
      }
    }

    return 0;
  }

  
  int Frame::writeAccumulate(int fd, byte* data, unsigned short length) {
    accumulate(data, length);
  
    return fdwrite(fd, data, length);
  }

 int Frame::readAccumulate(int fd, byte* data, unsigned short length) {
    int result = fdread(fd, data, length);
    if (result < 0) {
      return result;
    }
    
    accumulate(data, length);
    
    return 0;
  }

  void Frame::accumulate(byte* data, unsigned short length) {
    for (int index = 0; index < length; index++) {
      checksum_ += data[index];
    }
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
