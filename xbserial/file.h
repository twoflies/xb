/***********************************************************/
/* file                                                    */
/***********************************************************/

#ifndef _FILE_H_
#define _FILE_H_

typedef unsigned char byte;

int fdwrite(int fd, byte* data, int length = 1);
int _fdwrite(int fd, const byte* data, int length = 1);
int fdread(int fd, byte* data, int length = 1);
int _fdread(int fd, const byte* data, int length = 1);

#endif // _FILE_H_
