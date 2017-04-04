
#include "test.h"
#include "serial.h"

#include <string>
#include <iostream>

#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace XB;

int main(int argc, char **argv) {
  std::string dev("/dev/ttyUSB0");
  Serial serial;

  int result = serial.open(dev.c_str(), 9600);
  if (result != 0) {
    return handleError(result, "Failed to open");
  }
  
  Command* command = new Command("ID");
  result = serial.send(new RemoteCommandFrame(Address64::BROADCAST, Address16::UNKNOWN, (byte)0x02, command, (byte)0x01));
  if (result != 0) {
    return handleError(result, "Failed to send");
  }

  std::cout << "Sent Command" << std::endl;

  ResponseFrame *response;
  while ((response = serial.receiveAny()) != NULL) {
    std::cout << "Received Response" << std::endl;

    CommandResponseFrame *commandResponse = dynamic_cast<CommandResponseFrame*>(response);
    if (commandResponse != NULL) {
      byte *parameter = commandResponse->getParameter();
      int length = commandResponse->getLength();
      for (int index = 0; index < length; index++) {
	char buffer[10];
	snprintf(buffer, 10, "%02X-", parameter[index]);
	std::cout << buffer;
      }
      std::cout << std::endl;
    }
  }
  
  /*XBRemoteCommandResponseFrame response;
  result = serial.receive(&response);
  if (result != 0) {
    return handleError(result, "Failed to receive");
  }

  std::cout << "Received Command" << std::endl;

  byte *parameter = response.getParameter();
  int length = response.getLength();
  for (int index = 0; index < length; index++) {
    char buffer[10];
    snprintf(buffer, 10, "%02X-", parameter[index]);
    std::cout << buffer;
  }
  std::cout << std::endl;*/ 
  
  return serial.close();
}

int handleError(int err, const char *message) {
  std::cout << message << ": " << err;
  if (err == -1) {
    std::cout << " (" << strerror(errno) << ")";
  }
  std::cout << std::endl;

  return err;
}
