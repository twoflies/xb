/***********************************************************/
/* frame                                                   */
/***********************************************************/

#ifndef _FRAME_H_
#define _FRAME_H_

#include <string>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "file.h"

namespace XB {

  const bool DEBUG_FRAMES = false;

  const byte TYPE_COMMAND = ((byte)0x08);
  const byte TYPE_COMMAND_RESPONSE = ((byte)0x88);
  const byte TYPE_REMOTE_COMMAND = ((byte)0x17);
  const byte TYPE_REMOTE_COMMAND_RESPONSE = ((byte)0x97);
  const byte TYPE_IO_SAMPLE = 0x92;

  class FrameHeader {
  public:
    FrameHeader();
    int read(int fd, long timeout = 0);
    unsigned short getLength() const;
    byte getType() const;

  public:
    static std::string getTypeCode(byte type) {
      static char buffer[5];
      switch (type) {
      case TYPE_COMMAND:
	return " C ";
      case TYPE_COMMAND_RESPONSE:
	return " CR";
      case TYPE_REMOTE_COMMAND:
	return "RC ";
      case TYPE_REMOTE_COMMAND_RESPONSE:
	return "RCR";
      case TYPE_IO_SAMPLE:
	return "IO ";
      default:
	snprintf(buffer, 4, "%02X", type);
	return buffer;
      }
    }

  protected:
    unsigned short length_;
    byte type_;
  };

  
  const int ERROR_CHECKSUM = -42;

  const byte STATUS_OK = 0;
  const byte STATUS_ERROR = 0x01;
  const byte STATUS_INVALID_COMMAND = 0x02;
  const byte STATUS_INVALID_PARAMETER = 0x03;
  const byte STATUS_TX_FAILURE = 0x04;
  
  class Frame {
  public:
    Frame(byte type);
    Frame(FrameHeader* header);
    virtual ~Frame();
    byte getType() const;
    int write(int fd);
    int read(int fd);
    int readFromHeader(int fd, FrameHeader* header);

  protected:
    virtual byte getStatus() const;
    int writeHeader(int fd);
    virtual unsigned short getPayloadPrologueLength();
    virtual unsigned short getPayloadLength();
    virtual int writePayloadPrologue(int fd);
    virtual int writePayload(int fd);
    int writeChecksum(int fd);
    int readHeader(int fd, FrameHeader* header);
    virtual int readPayloadPrologue(int fd);
    virtual int readPayload(int fd, unsigned short length);
    int readChecksum(int fd);
    int writeAccumulate(int fd, byte* data, unsigned short length = 1);
    int readAccumulate(int fd, byte* data, unsigned short length = 1);
    void accumulate(byte* data, unsigned short length = 1);
    
  protected:
    static byte start_;
    byte type_;
    byte checksum_;
  };


  int _logData(unsigned char* data, unsigned short length = 1);
  int logData(unsigned char* data, unsigned short length = 1);
}

#endif // _FRAME_H_
