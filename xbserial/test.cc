
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
  Module module(Address64(0x00, 0x13, 0xA2, 0x00, 0x41, 0x46, 0xB5, 0xA9), UNKNOWN, std::string());
  //result = serial.setParameter(Command("SP"), Parameter(0x5DC));
  //result = serial.setParameter(Command("SN"), Parameter(0x14));
  //result = serial.getRemoteParameter(&module, Command("NI"), &parameter);

  result = serial.setRemoteParameter(&module, Command("ST"), Parameter(0x1388), OPTION_APPLY);
  result = serial.setRemoteParameter(&module, Command("IR"), Parameter((unsigned short)0), OPTION_APPLY);

  //result = serial.setRemoteParameter(&module, Command("SM"), Parameter(0x04));
  result = serial.setRemoteParameter(&module, Command("V+"), Parameter(0xFFFF));
  result = serial.setRemoteParameter(&module, Command("IR"), Parameter(0x32));
  result = serial.setRemoteParameter(&module, Command("SP"), Parameter(0x5DC));
  result = serial.setRemoteParameter(&module, Command("SN"), Parameter(0x14));
  result = serial.setRemoteParameter(&module, Command("ST"), Parameter(0x32));
  result = serial.sendRemoteCommand(&module, Command("AC"));
  if (result != 0) {
    return logError(result, "Failed to send");
  }

  // [95 13-A2-00-41-46-B5-A9-A5-0F-42-A5-0F-00-13-A2-00-41-46-B5-A9-20-00-00-00-02-03-C1-05-10-1E]

  logData(parameter.data, parameter.length);
  log("%d", parameter.ushort());
    
  return serial.close();
}
