/***********************************************************/
/* iosample                                                */
/***********************************************************/

#ifndef _IO_SAMPLE_H_
#define _IO_SAMPLE_H_

#include <vector>

#include "frame.h"
#include "command.h"

namespace XB {

  typedef _2Byte DigitalMask;
  typedef _2Byte Sample;

  enum PIN {
    // Digital
    D0  = (1u << 0),
    D1  = (1u << 1),
    D2  = (1u << 2),
    D3  = (1u << 3),
    D4  = (1u << 4),
    D5  = (1u << 5),
    D6  = (1u << 6),
    D7  = (1u << 7),
    D10 = (1u << 10),
    D11 = (1u << 11),
    D12 = (1u << 12),
    // Analog
    A0 = (1u << 0),
    A1 = (1u << 1),
    A2 = (1u << 2),
    A3 = (1u << 3),
    // Voltage
    AV = (1u << 7)
  };

  const float VOLTAGE_SCALE = 1200.0f/1024;
  
  class IOSampleFrame : public Frame {
  public:
    IOSampleFrame(byte type = TYPE_IO_SAMPLE);
    IOSampleFrame(FrameHeader* header);
    virtual ~IOSampleFrame();
    Address64 getAddress64() const;
    Address16 getAddress16() const;
    byte getReceiveOptions() const;
    byte getSampleCount() const;
    DigitalMask getDigitalMask() const;
    byte getAnalogMask() const;
    Sample getDigitalSample() const;
    const std::vector<Sample>& getAnalogSamples() const;
    bool isDigitalPinEnabled(PIN pin) const;
    bool isDigitalPinSet(PIN pin) const;
    bool isAnalogPinEnabled(PIN pin) const;
    Sample getAnalogPinSample(PIN pin) const;
    unsigned short getVoltage() const;
    
  protected:
    virtual int readPayload(int fd, unsigned short length);
  
  private:
    Address64 address64_;
    Address16 address16_;
    byte receiveOptions_;
    byte sampleCount_;
    DigitalMask digitalMask_;
    byte analogMask_;
    Sample digitalSample_;
    std::vector<Sample> analogSamples_;
  };
}

#endif // _IO_SAMPLE_H_
