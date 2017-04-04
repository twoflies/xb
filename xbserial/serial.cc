/***********************************************************************/
/* Serial                                                              */
/***********************************************************************/

#include "serial.h"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "file.h"

namespace XB {
  
  Serial::Serial() {
    fd_ = -1;
  }

  Serial::Serial(const char *dev, int baud) {
    fd_ = -1;
    open(dev, baud);
  }

  int Serial::open(const char *dev, int baud) {
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

  int Serial::send(RequestFrame *frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }
  
    return frame->write(fd_);
  }

  int Serial::receive(ResponseFrame *frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    return frame->read(fd_);
  }

  int Serial::receive(FrameHeader *header, ResponseFrame *frame) {
    if (fd_ < 0) {
      return ERROR_NOPEN;
    }

    int result = frame->readPayload(fd_, header->getPayloadLength());

    return result;
  }

  ResponseFrame* Serial::receiveAny() {
    FrameHeader header;
    while (header.read(fd_) >= 0) {
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
	//char stype[5];
	//snprintf(stype, 5, "%02X", header.getType());
	//std::cout << "Received Unknown Type " << stype << std::endl;
	continue;
      }

      int result = receive(&header, frame);
      return (result == 0) ? frame : NULL;
    }

    return NULL;
  }

}
