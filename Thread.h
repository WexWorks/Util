// Copyright (c) 2013 by The 11ers, LLC -- All Rights Reserved

#ifndef THREAD_H_
#define THREAD_H_

/*
   Cross-platform thread, mutex, condition variables and atomic operations
   that work on Android, iOS, Linux, Windows and OSX.

   Portion of this code modified from OIIO (Modified BSD):
     Copyright 2008 Larry Gritz and the other authors and contributors.
     All Rights Reserved.

 */

#ifdef WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#    define VC_EXTRALEAN 1
#    define NOMINMAX
#    include "windows.h"
#    include <winbase.h>
#    pragma intrinsic(_InterlockedExchangeAdd)
#    pragma intrinsic(_InterlockedCompareExchange)
#    pragma intrinsic(_InterlockedCompareExchange64)
#    if defined(_WIN64)
#      pragma intrinsic(_InterlockedExchangeAdd64)
#    endif
#  endif
typedef HANDLE ThreadData;
typedef CRITICAL_SECTION MutexData;
typedef SRWLOCK RWLockData;
typedef CONDITION_VARIABLE  ConditionVariableData;
extern void *mtStartThread(void *data);
#else
#  include <pthread.h>
typedef pthread_t ThreadData;
typedef pthread_mutex_t MutexData;
typedef pthread_rwlock_t RWLockData;
typedef pthread_cond_t ConditionVariableData;
extern void *mtStartThread(void *data);
#endif

#ifdef __APPLE__
#  include <libkern/OSAtomic.h>
#endif

namespace mt {

class Thread {
public:
  Thread() {}
  virtual ~Thread() {
#if defined(WINDOWS)
    CloseHandle(thread_);
#endif
  }
  virtual bool Init() {
#if defined(WINDOWS)
    thread_ = CreateThread(NULL, 0, mtStartThread, this, 0, NULL);
    if (!thread_)
      return false;
    return true;
#else
    if (pthread_create(&thread_, NULL, mtStartThread, this))
      return false;
    return true;
#endif
  }
  virtual void SetName(const char *name) {
#if defined(WINDOWS)
    DWORD threadId = GetThreadId(thread_);
    if (threadId)
      SetThreadName(GetThreadId(thread_), name);
#else
    pthread_setname_np(name);
#endif
  }
  virtual void Run() = 0;                       // Override with main

private:
  Thread(const Thread &src);                    // Disallow copy ctor
  void operator=(const Thread&);                // Disallow assignment
  ThreadData thread_;
};

class Mutex {
public:
  Mutex() {
#if defined(WINDOWS)
    InitializeCriticalSection(&mutex_);
#else
    if (pthread_mutex_init(&mutex_, NULL))
      return;
#endif
  }
  ~Mutex() {
#if defined(WINDOWS)
    DeleteCriticalSection(&mutex_);
#else
    pthread_mutex_destroy(&mutex_);
#endif
  }
  bool Lock() {
#if defined(WINDOWS)
    EnterCriticalSection(&mutex_);
#else
    if (pthread_mutex_lock(&mutex_))
      return false;
#endif
    return true;
  }
  bool TryLock() {
#if defined(WINDOWS)
    if (TryEnterCriticalSection(&mutex) == 0)
      return false;
#else
    if (pthread_mutex_trylock(&mutex_))
      return false;
#endif
    return true;
  }
  bool Unlock() {
#if defined(WINDOWS)
    LeaveCriticalSection(&mutex_);
#else
    if (pthread_mutex_unlock(&mutex_))
      return false;
#endif
    return true;
  }

private:
  friend class ConditionVariable;
  Mutex(const Mutex&);                          // Disallow copy ctor
  void operator=(const Mutex&);                 // Disallow assignment
  MutexData mutex_;
};

class RWLock {
public:
  RWLock() {
#if defined(WINDOWS)
    InitializeSRWLock(&lock_);
#else
    if (pthread_rwlock_init(&lock_, NULL))
      return;
#endif
  }
  ~RWLock() {
#if defined(WINDOWS)
#else
    pthread_rwlock_destroy(&lock_);
#endif
  }
  bool ReadLock() {
#if defined(WINDOWS)
    AcquireSRWLockShared(&lock_);
#else
    if (pthread_rwlock_rdlock(&lock_))
      return false;
#endif
    return true;
  }
  bool ReadUnlock() {
#if defined(WINDOWS)
    ReleaseSRWLockShared(&lock_);
#else
    if (pthread_rwlock_unlock(&lock_))
      return false;
#endif
    return true;
  }
  bool WriteLock() {
#if defined(WINDOWS)
    AcquireSRWLockExclusive(&lock_);
#else
    if (pthread_rwlock_wrlock(&lock_))
      return false;
#endif
    return true;
  }
  bool WriteUnlock() {
#if defined(WINDOWS)
    ReleaseSRWLockExclusive(&lock_);
#else
    if (pthread_rwlock_unlock(&lock_))
      return false;
#endif
    return true;
  }

private:
  RWLock(const Mutex&);                         // Disallow copy ctor
  void operator=(const RWLock&);                // Disallow assignment
  RWLockData lock_;
};

template <class T> class LockGuard {
public:
  LockGuard(T &mutex) : mutex_(mutex) { mutex_.Lock(); }
  ~LockGuard() { mutex_.Unlock(); }
private:
  LockGuard();
  LockGuard(const LockGuard&);                  // Disallow copy ctor
  void operator=(const LockGuard&);             // Disallow assignment
  T &mutex_;
};

typedef LockGuard<Mutex> MutexLockGuard;

template <class T> class ReadLockGuard {
public:
  ReadLockGuard(T &lock) : lock_(lock) { lock_.ReadLock(); }
  ~ReadLockGuard() { lock_.ReadUnlock(); }
private:
  ReadLockGuard();                              // Disallow empty ctor
  ReadLockGuard(const ReadLockGuard&);          // Disallow copy ctor
  void operator=(const ReadLockGuard&);         // Disallow assignment
  T &lock_;
};

typedef ReadLockGuard<RWLock> ReadRWLockGuard;

template <class T> class WriteLockGuard {
public:
  WriteLockGuard(T &lock) : lock_(lock) { lock_.WriteLock(); }
  ~WriteLockGuard() { lock_.WriteUnlock(); }
private:
  WriteLockGuard();                             // Disallow empty ctor
  WriteLockGuard(const WriteLockGuard&);        // Disallow copy ctor
  void operator=(const WriteLockGuard&);        // Disallow assignment
  T &lock_;
};

typedef WriteLockGuard<RWLock> WriteRWLockGuard;

// Block in Wait and signal using Notify.
// Lock mutex and check condition and then call Wait to block.
// Mutex is unlocked inside of Wait and locked upon release.
class ConditionVariable {
public:
  ConditionVariable() {
#if defined(WINDOWS)
    InitializeConditionVariable(&cond_);
#else
    pthread_cond_init(&cond_, NULL);
#endif
  }
  ~ConditionVariable() {
#if defined(WINDOWS)
#else
    pthread_cond_destroy(&cond_);
#endif
  }
  void Wait(Mutex &mutex) {
#if defined(WINDOWS)
    if (!SleepConditionVariableCS(&cond_, &mutex.mutex_, INFINITE))
      return;
#else
    if (pthread_cond_wait(&cond_, &mutex.mutex_))
      return;
#endif
  }
  void NotifyOne() {
#if defined(WINDOWS)
    WakeAllConditionVariable(&cond_);
#else
    if (pthread_cond_signal(&cond_))
      return;
#endif
  }
  void NotifyAll() {
#if defined(WINDOWS)
    WakeConditionVariable(&cond_);
#else
    if (pthread_cond_broadcast(&cond_))
      return;
#endif
  }

private:
  ConditionVariable(const ConditionVariable&);  // Disallow copy ctor
  void operator=(const ConditionVariable&);     // Disallow assignment
  ConditionVariableData cond_;
  Mutex mutex_;
};

// Replace atom with rhs if atom==comp.  Return true if swapped
inline bool AtomicCAS(volatile int *atom, int comp, int rhs) {
#if defined(_GLIBCXX_ATOMIC_BUILTINS) || (__GNUC__*100+__GNUC_MINOR__ >= 401)
  return __sync_bool_compare_and_swap(atom, comp, rhs);
#elif defined(WINDOWS)
  return (_InterlockedCompareExchange((volatile LONG *)atom,
                                      newval, comp) == comp);
#elif defined(__APPLE__)
  return OSAtomicCompareAndSwap32Barrier(comp, rhs, atom);
#else
#  error Missing atomic functions
#endif
}

inline bool AtomicCAS(volatile float *atom, float comp, float rhs) {
  return AtomicCAS(*(int **)&atom, *(int *)&comp, *(int *)&rhs);
}

inline bool AtomicCAS(volatile long long *atom, long long comp, long long rhs) {
#if defined(_GLIBCXX_ATOMIC_BUILTINS) || (__GNUC__*100+__GNUC_MINOR__ >= 401)
  return __sync_bool_compare_and_swap(atom, comp, rhs);
#elif defined(WINDOWS)
  return (_InterlockedCompareExchange64((volatile LONGLONG *)atom,
                                        rhs, comp) == comp);
#elif defined(__APPLE__)
  return OSAtomicCompareAndSwap64Barrier(comp, rhs, atom);
#else
#  error Missing atomic functions
#endif
}

inline bool AtomicCAS(volatile double *atom, double comp, double rhs) {
  return AtomicCAS(*(long long **)&atom, *(long long *)&comp,
                   *(long long *)&rhs);
}

// v = atom, atom += rhs, return v;
inline int AtomicAdd(volatile int *atom, int rhs) {
#if defined(_GLIBCXX_ATOMIC_BUILTINS) || (__GNUC__*100+__GNUC_MINOR__ >= 401)
  return __sync_fetch_and_add((int *)atom, rhs);
#elif defined(WINDOWS)
  return _InterlockedExchangeAdd((volatile LONG *)atom, rhs);
#elif defined(OSX)
  return OSAtomicAdd32Barrier(rhs, atom) - rhs; // Intel perf?
#elif defined(IOS)
  return 
#else
#  error Missing atomic functions
#endif
}

inline float AtomicAdd(volatile float *atom, float rhs) {
  float v, r;
  do {
    v = *atom;
    r = v + rhs;
  } while (!AtomicCAS(atom, v, r));
  return v;
}

inline long long AtomicAdd(volatile long long *atom, long long rhs) {
#if defined(ANDROID) || defined(IOS)
  // Workaround lack of 64bit atomics on ARM using a hardcoded 32bit spin lock
  // Note: Passes GCC atomic #if-checks, but fails to link (2.3, v7) with:
  //       "undefined __sync_fetch_and_add_8"
  static volatile int lock;
  while (!AtomicCAS(&lock, 0, 1)) /*EMPTY*/;
  long long r = *atom;
  *atom += rhs;
  while (!AtomicCAS(&lock, 1, 0)) /*EMPTY*/;
  return r;
#elif defined(_GLIBCXX_ATOMIC_BUILTINS) || (__GNUC__*100+__GNUC_MINOR__ >= 401)
  return __sync_fetch_and_add(atom, rhs);
#elif defined(WINDOWS)
#if defined(_WIN64)
  return _InterlockedExchangeAdd64((volatile LONGLONG *)atom, rhs);
#else
  return InterlockedExchangeAdd64((volatile LONGLONG *)atom, rhs);
#endif
#elif defined(OSX)
  return OSAtomicAdd64Barrier(rhs, atom) - rhs;
#else
#  error Missing atomic functions
#endif
}

inline float AtomicAdd(volatile double *atom, double rhs) {
  double v, r;
  do {
    v = *atom;
    r = v + rhs;
  } while (!AtomicCAS(atom, v, r));
  return v;
}

// Atomic integer types, with support for basic type operations
template <class T> class Atomic {
public:
  Atomic() : val_(0) {}
  Atomic(const T val) : val_(val) {}
  T operator()() const { return AtomicAdd(&val_, 0); }
  operator T() const { return AtomicAdd(&val_, 0); }
  T operator=(T rhs) {
    while (1) { T r = val_; if (AtomicCAS(&val_, r, rhs)) break; } return rhs;
  }
  T operator++() { return AtomicAdd(&val_, 1) + 1; }
  T operator++(int) { return AtomicAdd(&val_, 1); }
  T operator--() { return AtomicAdd(&val_, -1) - 1; }
  T operator--(int) { return AtomicAdd(&val_, -1); }
  T operator+=(T rhs) { return AtomicAdd(&val_, rhs) + rhs; }
  T operator-=(T rhs) { return AtomicAdd(&val_, -rhs) - rhs; }
  bool CAS(T comp, T rhs) { return AtomicCAS(&val_, comp, rhs); }
  T operator=(const Atomic &rhs) { T r = rhs(); *this = r; return r; }

private:
  Atomic(const Atomic &);
  volatile mutable T val_;
};

typedef Atomic<int> AtomicInt;
typedef Atomic<long long> AtomicLongLong;
typedef Atomic<float> AtomicFloat;
typedef Atomic<double> AtomicDouble;

// SpinLock, a lock-free mutex implemented via atomics.
// Smaller and faster than a regular mutex, but burns CPU while waiting.
// Only use if you will hold a lock for a very short time.
//
// NOTE: Avoid having two spin locks within 128 bytes (same cacheline),
//       which can result in "false sharing".
class SpinLock {
public:
  SpinLock(void) : lock_(0) {}
  ~SpinLock (void) {}
  void Lock() {
#if defined(__APPLE__)
    OSSpinLockLock((OSSpinLock *)&lock_);
#else
    while (!TryLock()) /*EMPTY*/;
#endif
  }
  bool TryLock() {
#if defined(__APPLE__)
    return OSSpinLockTry((OSSpinLock *)&lock_);
#else
    return lock_.CAS(0, 1);
#endif
  }
  void Unlock () {
#if defined(__APPLE__)
    OSSpinLockUnlock((OSSpinLock *)&lock_);
#else
    lock_ = 0;
#endif
  }

private:
  SpinLock (const SpinLock &);                  // Disallow copy
  const SpinLock& operator=(const SpinLock&);   // Disallow assignment
  AtomicInt lock_;                              // Zero = unlocked
};

typedef LockGuard<SpinLock> SpinLockGuard;

class RWSpinLock {
public:
  RWSpinLock(void) : read_count_(0) {}
  ~RWSpinLock(void) {}
  void ReadLock() {
    lock_.Lock();                               // Spin until no active writers
    ++read_count_;                              // Register ourself as a reader
    lock_.Unlock();                             // Release to let readers work
  }
  void ReadUnlock () {
    --read_count_;                              // Atomic, so no need to lock
  }
  void WriteLock () {
    lock_.Lock();                               // Prevent new readers/writers
    while (read_count_ > 0) /*EMPTY*/;          // Spin until we are sole owner
  }
  void WriteUnlock () {
    lock_.Unlock ();                            // Allow others to lock
  }

private:
  const RWSpinLock& operator=(const RWSpinLock&);// Disallow assignment?
  RWSpinLock(const RWSpinLock &);               // Disallow copy
  SpinLock lock_;                               // Write lock
  AtomicInt read_count_;                        // # of readers
};

typedef ReadLockGuard<RWSpinLock> ReadRWSpinLockGuard;
typedef WriteLockGuard<RWSpinLock> WriteRWSpinLockGuard;

} // namespace mt

#endif  // THREAD_H_
