/***********************************************************/
/* frame                                                   */
/***********************************************************/

#ifndef _FRAME_H_
#define _FRAME_H_

#include <ostream>
#include <stddef.h>

#include "file.h"

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

  protected:
    int length_;
    byte type_;
    byte id_;
  };

  class Frame {
  public:
    virtual ~Frame();
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
    virtual ~ResponseFrame();
    int read(int fd);
    int readHeader(int fd, FrameHeader* header);
    virtual int readPayload(int fd, int length) = 0;
    int readAndValidateChecksum(int fd);

  protected:
    ResponseFrame(byte type, byte id);
    void accumulate(byte* data, int length = 1);
    int readAccumulate(int fd, byte* data, int length = 1);
  };


  struct Command {
    byte a, b;

    Command() {
      a = (byte)0;
      b = (byte)0;
    }

    Command(const char *command) {
      a = (byte)command[0];
      b = (byte)command[1];
    }
  };

  class CommandFrame : public RequestFrame {
  public:
    CommandFrame(Command* command, byte id);
    CommandFrame(Command* command, byte* parameter = NULL, int length = 0, byte id = 0);
    virtual ~CommandFrame();

  protected:
    CommandFrame(byte type, Command* command, byte* parameter = NULL, int length = 0, byte id = 0);
    virtual int getPayloadLength();
    virtual int writePayload(int fd);

  private:
    Command *command_;
    byte *parameter_;
    int length_;
  };


  class CommandResponseFrame : public ResponseFrame {
  public:
    CommandResponseFrame(byte type = TYPE_COMMAND_RESPONSE);
    CommandResponseFrame(FrameHeader* header);
    virtual ~CommandResponseFrame();
    Command* getCommand();
    byte* getParameter();
    int getLength();

  protected:
    virtual int readPayload(int fd, int length);
  
  private:
    Command* command_;
    byte status_;
    byte* parameter_;
    int length_;
  };


  struct Address64 {
    byte a, b, c, d, e, f, g, h;

    static Address64 *COORDINATOR;
    static Address64 *BROADCAST;
    
    Address64() {
      this->a = (byte)0x0;
      this->b = (byte)0x0;
      this->c = (byte)0x0;
      this->d = (byte)0x0;
      this->e = (byte)0x0;
      this->f = (byte)0x0;
      this->g = (byte)0x0;
      this->h = (byte)0x0;
    }
    
    Address64(byte a, byte b, byte c, byte d, byte e, byte f, byte g, byte h) {
      this->a = a;
      this->b = b;
      this->c = c;
      this->d = d;
      this->e = e;
      this->f = f;
      this->g = g;
      this->h = h;
    }
  };
  
  struct Address16 {
    byte a, b;
    
    static Address16 *UNKNOWN;
    
    Address16() {
      this->a = (byte)0x0;
      this->b = (byte)0x0;
    }
    
    Address16(byte a, byte b) {
      this->a = a;
      this->b = b;
    }
  };

  const byte OPTION_APPLY = ((byte)0x02);

  class RemoteCommandFrame : public CommandFrame {
  public:
    RemoteCommandFrame(Address64* address64, Address16* address16, byte options, Command* command, byte id);
    RemoteCommandFrame(Address64* address64, Address16* address16, byte options, Command* command, byte* parameter = NULL, int length = 0, byte id = 0);
    virtual ~RemoteCommandFrame();

  protected:
    int getPayloadLength();
    int writePayload(int fd);

  private:
    Address64* address64_;
    Address16* address16_;
    byte options_;
  };


  class RemoteCommandResponseFrame : public CommandResponseFrame {
  public:
    RemoteCommandResponseFrame(byte type = TYPE_REMOTE_COMMAND_RESPONSE);
    RemoteCommandResponseFrame(FrameHeader *header);
    virtual ~RemoteCommandResponseFrame();
    Address64* getAddress64();
    Address16* getAddress16();

  protected:
    virtual int readPayload(int fd, int length);
    
  private:
    Address64* address64_;
    Address16* address16_;
  };

}

#endif // _FRAME_H_
