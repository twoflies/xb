
#include "test.h"
#include "serial.h"

#include <string>
#include <iostream>

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "log.h"

using namespace XB;

int main(int argc, char **argv) {
  std::string dev("/dev/ttyUSB0");
  Serial serial;

  int result = serial.open(dev.c_str(), 9600);
  if (result != 0) {
    return logError(result, "Failed to open");
  }

  Parameter parameter;
  result = serial.getRemoteParameter(XBBROADCAST, XBUNKNOWN, XBCOMMAND("NT"), &parameter);
  if (result != 0) {
    return logError(result, "Failed to send");
  }

  logData((byte*)&parameter, 2);
    
  return serial.close();
}
