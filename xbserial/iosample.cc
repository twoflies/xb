/***********************************************************/
/* iosample                                                */
/***********************************************************/

#include "iosample.h"

#include <bitset>

#include "log.h"

namespace XB {

  IOSampleFrame::IOSampleFrame(byte type) : Frame(type) {
  }

  IOSampleFrame::IOSampleFrame(FrameHeader* header) : Frame(header) {
  }

  IOSampleFrame::~IOSampleFrame() {
  }

  Address64 IOSampleFrame::getAddress64() {
    return address64_;
  }

  Address16 IOSampleFrame::getAddress16() {
    return address16_;
  }

  byte IOSampleFrame::getReceiveOptions() {
    return receiveOptions_;
  }

  byte IOSampleFrame::getSampleCount() {
    return sampleCount_;
  }

  DigitalMask IOSampleFrame::getDigitalMask() {
    return digitalMask_;
  }

  byte IOSampleFrame::getAnalogMask() {
    return analogMask_;
  }

  Sample IOSampleFrame::getDigitalSample() {
    return digitalSample_;
  }

  std::vector<Sample> IOSampleFrame::getAnalogSamples() {
    return analogSamples_;
  }

  int IOSampleFrame::readPayload(int fd, unsigned short length) {
    int result = readAccumulate(fd, (byte*)&address64_, sizeof(address64_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(" @");
      _logData((byte*)&address64_, sizeof(address64_));
    }
      
    result = readAccumulate(fd, (byte*)&address16_, sizeof(address16_));
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log(",@");
      _logData((byte*)&address16_, sizeof(address16_));
    }
      
    result = readAccumulate(fd, &receiveOptions_);
    if (result != 0) {
      return result;
    }

    result = readAccumulate(fd, &sampleCount_);
    if (result != 0) {
      return result;
    }

    result = readAccumulate(fd, (byte*)&digitalMask_, sizeof(digitalMask_));
    if (result != 0) {
      return result;
    }

    _log(" ");
    _logData((byte*)&digitalMask_, sizeof(digitalMask_));

    result = readAccumulate(fd, &analogMask_);
    if (result != 0) {
      return result;
    }

    _log("/");
    _logData(&analogMask_);

    _log(" =");
    std::size_t digitalCount = std::bitset<16>(digitalMask_.ushort()).count();
    if (digitalCount > 0) {
      result = readAccumulate(fd, (byte*)&digitalSample_, sizeof(digitalSample_));
      if (result != 0) {
	return result;
      }

      _logData((byte*)&digitalSample_, sizeof(digitalSample_));
    }

    _log("/");
    std::size_t analogCount = std::bitset<8>(analogMask_).count();
    for (std::size_t index = 0; index < analogCount; index++) {
      Sample analogSample;
      result = readAccumulate(fd, (byte*)&analogSample, sizeof(analogSample));
      if (result != 0) {
	return result;
      }
      analogSamples_.push_back(analogSample);

      _logData((byte*)&analogSample, sizeof(analogSample));
    }

    return 0;
  }
}
