/***********************************************************/
/* file                                                    */
/***********************************************************/

#ifndef _FILE_H_
#define _FILE_H_

typedef unsigned char byte;

int fdwrite(int fd, byte* data, unsigned short length = 1);
int _fdwrite(int fd, const byte* data, unsigned short length = 1);
int fdread(int fd, byte* data, unsigned short length = 1);
int _fdread(int fd, const byte* data, unsigned short length = 1, long timeout = 0);

#endif // _FILE_H_
