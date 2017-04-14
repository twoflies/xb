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

  Address64 IOSampleFrame::getAddress64() const {
    return address64_;
  }

  Address16 IOSampleFrame::getAddress16() const {
    return address16_;
  }

  byte IOSampleFrame::getReceiveOptions() const {
    return receiveOptions_;
  }

  byte IOSampleFrame::getSampleCount() const {
    return sampleCount_;
  }

  DigitalMask IOSampleFrame::getDigitalMask() const {
    return digitalMask_;
  }

  byte IOSampleFrame::getAnalogMask() const {
    return analogMask_;
  }

  Sample IOSampleFrame::getDigitalSample() const {
    return digitalSample_;
  }

  const std::vector<Sample>& IOSampleFrame::getAnalogSamples() const {
    return analogSamples_;
  }

  bool IOSampleFrame::isDigitalPinEnabled(PIN pin) const {
    return ((digitalMask_.ushort() & pin) == pin);
  }
  
  bool IOSampleFrame::isDigitalPinSet(PIN pin) const {
    return isDigitalPinEnabled(pin) && ((digitalSample_.ushort() & pin) == pin);
  }

  bool IOSampleFrame::isAnalogPinEnabled(PIN pin) const {
    return ((analogMask_ & pin) == pin);
  }

  Sample IOSampleFrame::getAnalogPinSample(PIN pin) const {
    if (!isAnalogPinEnabled(pin)) {
      return Sample();
    }

    int index = 0;
    PIN mask = A0;
    while (mask != pin) {
      if (isAnalogPinEnabled(mask)) {
	index++;
      }
      mask = (PIN)(((int)mask) << 1);
    }
    return analogSamples_.at(index);
  }

  unsigned short IOSampleFrame::getVoltage() const {
    Sample sample = getAnalogPinSample(AV);
    return (unsigned short)(sample.ushort() * VOLTAGE_SCALE);
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

    if (DEBUG_FRAMES) {
      _log(" ");
      _logData((byte*)&digitalMask_, sizeof(digitalMask_));
    }
    
    result = readAccumulate(fd, &analogMask_);
    if (result != 0) {
      return result;
    }

    if (DEBUG_FRAMES) {
      _log("/");
      _logData(&analogMask_);
      _log(" =");
    }
    
    std::size_t digitalCount = std::bitset<16>(digitalMask_.ushort()).count();
    if (digitalCount > 0) {
      result = readAccumulate(fd, (byte*)&digitalSample_, sizeof(digitalSample_));
      if (result != 0) {
	return result;
      }

      if (DEBUG_FRAMES) {
	_logData((byte*)&digitalSample_, sizeof(digitalSample_));
      }
    }

    if (DEBUG_FRAMES) {
      _log("/");
    }
    
    std::size_t analogCount = std::bitset<8>(analogMask_).count();
    for (std::size_t index = 0; index < analogCount; index++) {
      Sample analogSample;
      result = readAccumulate(fd, (byte*)&analogSample, sizeof(analogSample));
      if (result != 0) {
	return result;
      }
      analogSamples_.push_back(analogSample);

      if (DEBUG_FRAMES) {
	_logData((byte*)&analogSample, sizeof(analogSample));
      }
    }

    return 0;
  }
}
