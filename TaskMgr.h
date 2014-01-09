//  Copyright (c) 2013 The 11ers. All rights reserved.

#ifndef TASKMGR_H
#define TASKMGR_H

#include <queue>
#include <vector>
#include <set>
#include <string>


namespace mt {

class ConditionVariable;                            // Inter-thread messaging
class Mutex;                                        // Protect data
class WorkerThread;                                 // Process tasks


// Individual task, derive custom types and add to manager for processing.
// Tasks are processed in priority order, higher priorities first.
  
class Task {
public:
  Task() {}
  virtual ~Task() {}
  virtual float Priority() const { return 0; }      // Ordering metric
  virtual bool operator()() = 0;                    // Override with action
  virtual bool operator<(const Task &rhs) const {   // Heap ordering
    return Priority() < rhs.Priority();             //   highest first
  }
  virtual const char *Name() const { return "Task"; } // Finding & debugging
};


// Compare two pointer types using operator< for heap.
// This is generally useful for sorting or heapifying any pointer type.
  
template<typename T, typename Compare = std::less<T> >
struct PtrLess : public std::binary_function<T *, T *, bool> {
  bool operator()(const T *x, const T *y) const { return Compare()(*x, *y); }
};

  
typedef std::priority_queue<Task*, std::vector<Task*>, PtrLess<Task> > TaskHeap;
typedef std::vector<WorkerThread *> ThreadVec;      // A list of worker threads
typedef std::multiset<std::string> TaskNameSet;


// Create a set of worker threads and a task deque.
// Tasks added are processed by worker threads in background.
  
class TaskMgr {
public:
  TaskMgr() : mMutex(NULL), mDone(false) {}
  virtual ~TaskMgr();
  
  virtual bool Init(size_t workerCount);            // Create threads & stuff
  virtual void Schedule(Task *task);                // Post for processing
  virtual Task *WaitForTask();                      // Called by worker threads
  virtual bool IsPending(const char *name);         // Task::Name is scheduled
  virtual bool Dormant() const;                     // No pending tasks
  virtual bool Delete(const WorkerThread &thread);  // Remove thread (in Worker)
  
private:
  TaskMgr(const TaskMgr &);                         // Disallow copy
  void operator=(const TaskMgr &);                  // Disallow assignment
  
  TaskHeap mTaskHeap;                               // Pending tasks
  TaskNameSet mTaskNameSet;                         // Pending tasks
  Mutex *mMutex;                                    // Protect deque
  ThreadVec mWorkerThreadVec;                       // Worker threads
  ConditionVariable *mNewWorkCond;                  // Worker communication
  bool mDone;                                       // Block until non-zero
};


}       // namespace mt

#endif /* defined(TASKMGR_H) */
