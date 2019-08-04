#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "main.h"

int              force_format;

int xioctl(int fh, int request, void *arg);
int InitDevice(void);
int OpenDevice(void);
int StartCapturing(void);
int StopCapturing(void);
int UninitDevice(void);
int CloseDevice(void);

#endif
