#ifndef __DEADLINE_H__
#define __DEADLINE_H__

#include "main.h"

float WCET_SCHED(void);
float Average_SCHED(void);
float BCET_SCHED(void);
void DisplayTaskTimes(void);

float Average(int thread);
float BCET(int thread);
float WCET(int thread);
float GetTime(struct timespec* value);
float ExecutionTimeCal(struct timespec* start, struct timespec* stop);
void DisplayAllDeadlines(void);
void SortThreadTime(int thread);
void ProbabilityDistribution(int thread);
void GetValues(int thread);

struct difference;

#endif
