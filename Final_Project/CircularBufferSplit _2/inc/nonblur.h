#ifndef __NONBLUR_H__
#define __NONBLUR_H__

#include "main.h"

unsigned char STORE_GRAY[TEST_FRAMES][(640*480)];
int FrameCheck(void);
int DisplayFrameDiff(void);
void FrameCorrection(void);
void StoreFrameCheck(void);
void FrameDelay(void);


#endif
