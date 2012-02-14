// Copyright (c) 2011 by WexWorks, LLC -- All Rights Reserved

#include "Timer.h"

#include <stdio.h>
#include <string.h>


Timer::Timer() {
  Restart();
}


double Timer::Elapsed() const {
  TimerVal end_time;
#if defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix)
  gettimeofday(&end_time, NULL);
  return end_time.tv_sec - start_time_.tv_sec + 0.000001 * (end_time.tv_usec
      - start_time_.tv_usec);
#endif
#if defined(_WIN64) || defined(_WIN32)
  QueryPerformanceCounter(&end_time);
  LARGE_INTEGER countsPerSecond;
  QueryPerformanceFrequency(&countsPerSecond);
  return (end_time.QuadPart - start_time_.QuadPart) /
  double(countsPerSecond.QuadPart);
#endif
}


void Timer::Restart() {
#if defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix)
  gettimeofday(&start_time_, NULL);
#endif
#if defined(_WIN64) || defined(_WIN32)
  QueryPerformanceCounter(&start_time_);
#endif
}


const char *
Timer::String(double seconds) {
  static int buf_num = 0;
  static const int buf_count = 8;
  static char buffer[buf_count][1024];
  int b = buf_num++ % buf_count;

  buffer[b][0] = '\0';

  // Check for invalid input
  if (seconds < 0) {
    strcpy(buffer[b], "< 0s");
    return buffer[b];
  }

  if (seconds > 365 * 60 * 60 * 24) {
    strcpy(buffer[b], "> 1 yr");
    return buffer[b];
  }

  int days, hours, minutes;
  days = int(seconds / (60 * 60 * 24));
  seconds -= days * (60 * 60 * 24);
  hours = int(seconds / (60 * 60));
  seconds -= hours * (60 * 60);
  minutes = int(seconds / 60);
  seconds -= minutes * 60;

  if (days > 0) {
    sprintf(buffer[b], "%dd %dh %dm", days, hours, minutes);
  } else if (hours > 0) {
    sprintf(buffer[b], "%dh %dm %ds", hours, minutes, int(seconds));
  } else if (minutes > 0) {
    sprintf(buffer[b], "%dm %ds", minutes, int(seconds));
  } else {
    sprintf(buffer[b], "%.4fs", seconds);
  }

  return buffer[b];
}

