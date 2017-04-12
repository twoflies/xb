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
  
  class IOSampleFrame : public Frame {
  public:
    IOSampleFrame(byte type = TYPE_IO_SAMPLE);
    IOSampleFrame(FrameHeader* header);
    virtual ~IOSampleFrame();
    Address64 getAddress64();
    Address16 getAddress16();
    byte getReceiveOptions();
    byte getSampleCount();
    DigitalMask getDigitalMask();
    byte getAnalogMask();
    Sample getDigitalSample();
    std::vector<Sample> getAnalogSamples();
    
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
