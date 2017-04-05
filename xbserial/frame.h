/***********************************************************/
/* frame                                                   */
/***********************************************************/

#ifndef _FRAME_H_
#define _FRAME_H_

#include <string>
#include <stddef.h>
#include <string.h>

#include "file.h"

#define XBCOMMAND(c)  (((unsigned short)c[0] & (unsigned short)0xFF) | (((unsigned short)c[1] << 8) & (unsigned short)0xFF00))
#define XBCOMMANDSTR(c)  (std::string() + (char)c + (char)(c >> 8))

#define XBPARAMETER(p)  (((unsigned short)p[0] & (unsigned short)0xFF) | (((unsigned short)p[1] << 8) & (unsigned short)0xFF00))
#define XBPARAMETERSTR(p)  (std::string() + (char)p + (char)(p >> 8))

#define XBADDRESS64(a,b,c,d,e,f,g,h)  (((unsigned long long)a & (unsigned long long)0xFF) | (((unsigned long long)b << 8) & (unsigned long long)0xFF00) | (((unsigned long long)c << 16) & (unsigned long long)0xFF0000) | (((unsigned long long)d << 24) & (unsigned long long)0xFF000000) | (((unsigned long long)e << 32) & (unsigned long long)0xFF00000000) | (((unsigned long long)f << 40) & (unsigned long long)0xFF0000000000) | (((unsigned long long)g << 48) & (unsigned long long)0xFF000000000000) | (((unsigned long long)h << 56) & (unsigned long long)0xFF00000000000000))

#define XBADDRESS16(a,b)  (((byte)a & (unsigned long long)0xFF) | (((byte)b << 8) & (unsigned long long)0xFF00))

#define XBCOORDINATOR  XBADDRESS64(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
#define XBBROADCAST  XBADDRESS64(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF)
#define XBUNKNOWN XBADDRESS16(0xFF, 0xFE)


namespace XB {

  const byte TYPE_COMMAND = ((byte)0x08);
  const byte TYPE_COMMAND_RESPONSE = ((byte)0x88);
  const byte TYPE_REMOTE_COMMAND = ((byte)0x17);
  const byte TYPE_REMOTE_COMMAND_RESPONSE = ((byte)0x97);

  class FrameHeader {
  public:
    FrameHeader();
    int read(int fd);
    int getLength();
    int getPayloadLength();
    byte getType();
    byte getId();

  public:
    static std::string getTypeCode(byte type) {
      switch (type) {
      case TYPE_COMMAND:
	return " C ";
      case TYPE_COMMAND_RESPONSE:
	return " CR";
      case TYPE_REMOTE_COMMAND:
	return "RC ";
      case TYPE_REMOTE_COMMAND_RESPONSE:
	return "RCR";
      default:
	return "UNK";
      }
    }

  protected:
    int length_;
    byte type_;
    byte id_;
  };

  class Frame {
  public:
    virtual ~Frame();
    byte getType();
    byte getId();
    static byte start_;
  
  protected:
    Frame(byte type, byte id = 0);
    Frame(FrameHeader *header);
    byte type_;
    byte id_;
    byte checksum_;
  };
  

  class RequestFrame : public Frame {
  public:
    RequestFrame(byte type, byte id = 0);
    virtual ~RequestFrame();
    int write(int fd);

  protected:
    int writeAccumulate(int fd, byte* data, int length = 1);
    virtual int getPayloadLength() = 0;
    virtual int writePayload(int fd) = 0;
  };


  class ResponseFrame : public Frame {
  public:
    ResponseFrame(byte type);
    ResponseFrame(FrameHeader* header);
    virtual ~ResponseFrame();
    int read(int fd);
    int readHeader(int fd, FrameHeader* header);
    virtual int readPayload(int fd, int length);
    int readAndValidateChecksum(int fd);

  protected:
    void accumulate(byte* data, int length = 1);
    int readAccumulate(int fd, byte* data, int length = 1);
  };


  typedef unsigned short Command;
  typedef unsigned short Parameter;

  class CommandFrame : public RequestFrame {
  public:
    CommandFrame(Command command, byte id = 0);
    CommandFrame(Command command, Parameter parameter, byte id = 0);
    CommandFrame(Command command, byte* data, int length, byte id = 0);
    virtual ~CommandFrame();

  protected:
    CommandFrame(byte type, Command command, byte id = 0);
    CommandFrame(byte type, Command command, Parameter parameter, byte id = 0);
    CommandFrame(byte type, Command command, byte* data, int length, byte id = 0);
    virtual int getPayloadLength();
    virtual int writePayload(int fd);

  private:
    Command command_;
    Parameter parameter_;
    byte *data_;
    int length_;
  };


  class CommandResponseFrame : public ResponseFrame {
  public:
    CommandResponseFrame(byte type = TYPE_COMMAND_RESPONSE);
    CommandResponseFrame(FrameHeader* header);
    virtual ~CommandResponseFrame();
    Command getCommand();
    byte getStatus();
    byte* getData();
    int getLength();

  protected:
    virtual int readPayload(int fd, int length);
  
  private:
    Command command_;
    byte status_;
    byte* data_;
    int length_;
  };


  typedef unsigned long long Address64;
  typedef unsigned short Address16;
  
  struct Module {
    Address64 address64;
    Address16 address16;
    
    Module(Address64 address64, Address16 address16) {
      this->address64 = address64;
      this->address16 = address16;
    }
  };

  const byte OPTION_APPLY = ((byte)0x02);

  class RemoteCommandFrame : public CommandFrame {
  public:
    RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, byte id = 0);
    RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, Parameter parameter, byte id = 0);
    RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, byte* data, int length, byte id = 0);
    virtual ~RemoteCommandFrame();

  public:
    static RemoteCommandFrame* toCoordinator(Command command, byte id = 0);
    static RemoteCommandFrame* toCoordinator(Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);
    static RemoteCommandFrame* toBroadcast(Command command, byte id = 0);
    static RemoteCommandFrame* toBroadcast(Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);
    static RemoteCommandFrame* toModule(Module* module, Command command, byte id = 0);
    static RemoteCommandFrame* toModule(Module* module, Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);

  protected:
    int getPayloadLength();
    int writePayload(int fd);

  private:
    Address64 address64_;
    Address16 address16_;
    byte options_;
  };


  class RemoteCommandResponseFrame : public CommandResponseFrame {
  public:
    RemoteCommandResponseFrame(byte type = TYPE_REMOTE_COMMAND_RESPONSE);
    RemoteCommandResponseFrame(FrameHeader *header);
    virtual ~RemoteCommandResponseFrame();
    Address64 getAddress64();
    Address16 getAddress16();

  protected:
    virtual int readPayload(int fd, int length);
    
  private:
    Address64 address64_;
    Address16 address16_;
  };

  int _logData(unsigned char* data, int length = 1);
  int logData(unsigned char* data, int length = 1);
}

#endif // _FRAME_H_
