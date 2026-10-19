#ifndef WINNLS_H_SHIM
#define WINNLS_H_SHIM
#define CP_ACP 0
#define CP_OEMCP 1
#define CP_UTF8 65001
int MultiByteToWideChar(unsigned int, unsigned long, const char*, int, unsigned short*, int);
int WideCharToMultiByte(unsigned int, unsigned long, const unsigned short*, int, char*, int, const char*, int*);
#endif
