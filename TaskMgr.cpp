//  Copyright (c) 2013 The 11ers. All rights reserved.

#include "TaskMgr.h"

#include "Thread.h"

#include <assert.h>


using namespace mt;


//
// WorkerThread
//

class mt::WorkerThread : public Thread {
public:
  WorkerThread() : mTaskMgr(NULL) {}
  virtual bool Init(TaskMgr *taskMgr) {
    mTaskMgr = taskMgr;
    if (!Thread::Init())
      return false;
    return true;
  }
  
  virtual void Run() {
    while (1) {
      Task *task = mTaskMgr->WaitForTask();       // Wait for task
      if (!task) {
        printf("Worker received NULL task -- exiting\n");
        break;
      }
      SetName(task->Name());
      if (!(*task)())                             // Process task
        printf("Task error\n");
      delete task;                                // Clean up memory
    }
    mTaskMgr->Delete(*this);                      // Remove from mgr
    delete this;                                  // SUICIDE!
  }
  
private:
  TaskMgr *mTaskMgr;
};


//
// TaskMgr
//

TaskMgr::~TaskMgr() {
  mMutex->Lock();
  while (!mTaskHeap.empty()) {                    // Clear out remaining tasks
    Task *task = mTaskHeap.top();
    mTaskHeap.pop();
    delete task;
  }
  mTaskNameSet.clear();
  mDone = true;                                   // Signal workers
  mMutex->Unlock();
  while (!mWorkerThreadVec.empty())               // Wait for all to Delete
    mNewWorkCond->NotifyOne();                    // Threads delete themselves
  delete mMutex;
}


bool TaskMgr::Init(size_t workerCount) {
  mMutex = new Mutex;
  mNewWorkCond = new ConditionVariable;
  
  for (size_t i = 0; i < workerCount; ++i) {
    WorkerThread *wt = new WorkerThread;
    if (!wt->Init(this))
      return false;
    mWorkerThreadVec.push_back(wt);
  }
  return true;
}


void TaskMgr::Schedule(Task *task) {
  MutexLockGuard guard(*mMutex);
  mTaskHeap.push(task);
  mTaskNameSet.insert(task->Name());
  mNewWorkCond->NotifyOne();
  printf("Add: Task deque has %zd entries\n", mTaskHeap.size());
}


Task *TaskMgr::WaitForTask() {
  while (!mDone) {
    MutexLockGuard guard(*mMutex);
    if (mTaskHeap.empty()) {
      mNewWorkCond->Wait(*mMutex);
    } else {
      Task *task = mTaskHeap.top();
      mTaskHeap.pop();
      TaskNameSet::iterator n = mTaskNameSet.find(task->Name());
      assert(n != mTaskNameSet.end());
      mTaskNameSet.erase(n);
      printf("Pop: Task deque has %zd entries after \"%s\"\n", mTaskHeap.size(),
             task->Name());
      return task;
    }
  }
  
  return NULL;
}


bool TaskMgr::IsPending(const char *name) {
  mt::MutexLockGuard guard(*mMutex);
  return mTaskNameSet.find(name) != mTaskNameSet.end();
}


bool TaskMgr::Dormant() const {
  MutexLockGuard guard(*mMutex);
  return mTaskHeap.empty();
}


bool TaskMgr::Delete(const mt::WorkerThread &thread) {
  MutexLockGuard guard(*mMutex);
  for (ThreadVec::iterator i = mWorkerThreadVec.begin();
       i != mWorkerThreadVec.end(); ++i) {
    if (*i == &thread) {
      mWorkerThreadVec.erase(i);
      return true;
    }
  }
  return false;
}
