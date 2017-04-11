/***********************************************************/
/* command                                                 */
/***********************************************************/

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "frame.h"

namespace XB {
  
  struct _2Byte {
    byte a, b;

    _2Byte() {
      a = 0;
      b = 0;
    }

    _2Byte(const char *command) {
      a = (byte)command[0];
      b = (byte)command[1];
    }

    _2Byte(byte* data) {
      a = data[0];
      b = data[1];
    }

    _2Byte(byte a, byte b) {
      this->a = a;
      this->b = b;
    }

    std::string std_string() {
      return std::string() + (char)a + (char)b;
    }
  };

  typedef _2Byte Command;
  
  struct Parameter {
    byte* data;
    unsigned short length;
    unsigned short value;

    Parameter() {
      data = NULL;
      length = 0;
    }

    Parameter(unsigned short value) {
      this->value = htons(value);
      data = (byte*)&this->value;
      length = 2;
    }

    Parameter(char* parameter) {
      data = (byte*)parameter;
      length = strlen(parameter);
    }

    Parameter(byte* data, unsigned short length) {
      this->data = data;
      this->length = length;
    }

    std::string std_string() {
      return std::string((char*)data, length);
    }

    unsigned short ushort() {
      return ntohs((unsigned short)data[0] | ((unsigned short)data[1] << 8));
    }
  };
  
  class CommandFrame : public Frame {
  public:
    CommandFrame(Command command, byte id = 0);
    CommandFrame(Command command, Parameter parameter, byte id = 0);
    virtual ~CommandFrame();
    byte getId();

  protected:
    CommandFrame(byte type, Command command, byte id = 0);
    CommandFrame(byte type, Command command, Parameter parameter, byte id = 0);
    virtual unsigned short getPayloadLength();
    virtual int writePayloadPrologue(int fd);
    virtual int writePayload(int fd);

  private:
    byte id_;
    Command command_;
    Parameter parameter_;
  };


  class CommandResponseFrame : public Frame {
  public:
    CommandResponseFrame(byte type = TYPE_COMMAND_RESPONSE);
    CommandResponseFrame(FrameHeader* header);
    virtual ~CommandResponseFrame();
    byte getId();
    Command getCommand();
    virtual byte getStatus();
    Parameter getParameter();
    Parameter detachParameter();
    
  protected:
    virtual unsigned short getPayloadPrologueLength();
    virtual int readPayloadPrologue(int fd);
    virtual int readPayload(int fd, unsigned short length);
  
  private:
    byte id_;
    Command command_;
    byte status_;
    Parameter parameter_;
  };


  struct _8Byte {
    byte a, b, c, d, e, f, g, h;

    _8Byte() {
      a = 0;
      b = 0;
      c = 0;
      d = 0;
      e = 0;
      f = 0;
      g = 0;
      h = 0;
    }
    
    _8Byte(byte a, byte b, byte c, byte d, byte e, byte f, byte g, byte h) {
      this->a = a;
      this->b = b;
      this->c = c;
      this->d = d;
      this->e = e;
      this->f = f;
      this->g = g;
      this->h = h;
    }

     _8Byte(byte* data) {
      a = data[0];
      b = data[1];
      c = data[2];
      d = data[3];
      e = data[4];
      f = data[5];
      g = data[6];
      h = data[7];
    }
  };

  typedef _8Byte Address64;
  typedef _2Byte Address16;

  const Address64 COORDINATOR(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  const Address64 BROADCAST(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF);

  const Address16 UNKNOWN(0xFF, 0xFE);
  
  struct Module {
    Address64 address64;
    Address16 address16;
    std::string identifier;

    Module() {
    }
    
    Module(Address64 address64, Address16 address16, std::string identifier) {
      this->address64 = address64;
      this->address16 = address16;
      this->identifier = identifier;
    }
  };

  const byte OPTION_DISABLE_ACK = 0x01;
  const byte OPTION_APPLY = 0x02;
  const byte OPTION_EXTENDED_TX_TIMEOUT = 0x04;

  class RemoteCommandFrame : public CommandFrame {
  public:
    RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, byte id = 0);
    RemoteCommandFrame(Address64 address64, Address16 address16, byte options, Command command, Parameter parameter, byte id = 0);
    virtual ~RemoteCommandFrame();

  public:
    static RemoteCommandFrame* toCoordinator(Command command, byte id = 0);
    static RemoteCommandFrame* toCoordinator(Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);
    static RemoteCommandFrame* toBroadcast(Command command, byte id = 0);
    static RemoteCommandFrame* toBroadcast(Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);
    static RemoteCommandFrame* toModule(Module* module, Command command, byte id = 0);
    static RemoteCommandFrame* toModule(Module* module, Command command, Parameter parameter, byte id = 0, byte options = OPTION_APPLY);

  protected:
    unsigned short getPayloadLength();
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
    virtual int readPayload(int fd, unsigned short length);
    
  private:
    Address64 address64_;
    Address16 address16_;
  };

}

#endif  // _COMMAND_H
