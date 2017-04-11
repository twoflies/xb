/***********************************************************/
/* file                                                    */
/***********************************************************/
#include "file.h"

#include <algorithm>
#include <iterator>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

#include "log.h"

const byte ESCAPABLES[] = {(byte)0x11, (byte)0x13, (byte)0x7D, (byte)0x7E};
const int ESCAPABLES_COUNT = 4;
const byte ESCAPE_BYTE = ((byte)0x7D);
const byte ESCAPE_MASK = ((byte)0x20);

int _escindex(int index, const byte* data, unsigned short length);
int _compare(const void* a, const void* b);

int fdwrite(int fd, byte *data, unsigned short length) {
  int index = 0;
  while (index < length) {
    int next = _escindex(index, data, length);
    int result;
    if (next >= 0) {
      result = _fdwrite(fd, &data[index], next - index);
      if (result >= 0) {
	result = _fdwrite(fd, &ESCAPE_BYTE);
      }
      if (result >= 0) {
	byte escaped = data[next] ^ ESCAPE_MASK;
	result = _fdwrite(fd, &escaped);
      }
      next++;
    }
    else {
      result = _fdwrite(fd, &data[index], length - index);
      next = length;
    }

    if (result < 0) {
      return result;
    }
    else {
      index = next;
    }
  }

  return 0;
}

int _escindex(int index, const byte *data, unsigned short length) {
  for (; index < length; index++) {
    byte* esc = (byte*)bsearch(&(data[index]), ESCAPABLES, ESCAPABLES_COUNT, sizeof(*data), _compare);
    if (esc != NULL) {
      return index;
    }
  }
  
  return -1;
}

int _compare(const void* a, const void* b) {
  return *((byte*)a) - *((byte*)b);
}

int _fdwrite(int fd, const byte *data, unsigned short length) {
  if (length == 0) {
    return 0;
  }
  
  const byte *current = data;
  int remaining = length;
  int bytesWritten = 0;
  while ((bytesWritten = ::write(fd, (void*)current, (size_t)(remaining * sizeof(*data)))) >= 0) {
    remaining -= bytesWritten;
    current += bytesWritten * sizeof(*data);
    if (remaining <= 0) {
      break;
    }
  }

  for (int index = 0; index < length; index++) {
    //XB::_log("%02X ", data[index]);
  }
  //XB::log("(%d)", length);

  return (bytesWritten < 0) ? bytesWritten : 0;
}

int fdread(int fd, byte *data, unsigned short length) {
  bool escapeLast = false;
  int index = 0;
  while (index < length) {
    int result = _fdread(fd, &data[index], length - index);    
    if (result < 0) {
      return result;
    }

    if (escapeLast) {
      data[index] = data[index] ^ ESCAPE_MASK;
      break;
    }

    int alength = length;
    for (; index < alength; index++) {
      if (data[index] == ESCAPE_BYTE) {
	if (index < (alength - 1)) {
	  data[index] = data[index + 1] ^ ESCAPE_MASK;
	  if (index < (alength - 2)) {
	    memmove(&data[index + 1], &data[index + 2], (size_t)(alength - (index + 2)));
	  }
	}
	else {
	  escapeLast = true;
	}
	alength--;
      }
    }
    index = alength;
  }

  return 0;
}

int _fdread(int fd, const byte *data, unsigned short length, long timeout) {

  if (timeout > 0) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    struct timeval tout;
    tout.tv_sec = 0;
    tout.tv_usec = timeout * 1000;

    int result = select(fd + 1, &set, NULL, NULL, &tout);
    if (result == -1) {
      return result;
    }
    else if (result == 0) {
      return -2;
    }
    // else data available
  }
  
  const byte *current = data;
  int remaining = length;
  int bytesRead = 0;
  while ((bytesRead = ::read(fd, (void*)current, (size_t)(remaining * sizeof(*data)))) >= 0) {
    remaining -= bytesRead;
    current += bytesRead * sizeof(*data);
    if (remaining <= 0) {
      break;
    }
  }

  return (bytesRead < 0) ? bytesRead : 0;
}
