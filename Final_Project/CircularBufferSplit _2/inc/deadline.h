#ifndef __DEADLINE_H__
#define __DEADLINE_H__

#include "main.h"

float WCET(int thread);
float GetTime(struct timespec* value);
float ExecutionTimeCal(struct timespec* start, struct timespec* stop);
void DisplayAllDeadlines(void);

#endif
