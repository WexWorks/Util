// Copyright (c) 2011 by WexWorks, LLC -- All Rights Reserved

#include "Thread.h"


using namespace mt;


#ifdef WINDOWS

DWORD WINAPI mtStartThread(LPVOID data) {
  Thread *thread = static_cast<Thread *>(data);
  thread->Run();
  return NULL;
}

#else   // WINDOWS

void *mtStartThread(void *data) {
  Thread *thread = static_cast<Thread *>(data);
  thread->Run();
#if ANDROID
//  extern void DetachCurrentThread();
//  DetachCurrentThread();
#endif
  return NULL;
}


#endif  // !WINDOWS
