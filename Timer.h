// Copyright (c) 2012 by The 11ers, LLC -- All Rights Reserved

#ifndef TIMER_H
#define TIMER_H

#if defined(_WIN64) || defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN 1
#define NOMINMAX
#include "windows.h"
#endif
typedef LARGE_INTEGER TimerVal;
#endif

#if defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix)
#include <sys/time.h>
typedef struct timeval TimerVal;
#endif


class Timer {
public:
  Timer();
  ~Timer() {}

  double Elapsed() const;
  void Restart();

  static const char *String(double seconds);
  const char *String() const {
    return String(Elapsed());
  }

private:
  TimerVal mStartTime;
};


#endif   // TIMER_H_
