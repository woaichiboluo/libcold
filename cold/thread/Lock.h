#ifndef COLD_THREAD_LOCK
#define COLD_THREAD_LOCK

#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define RELEASE_GENERIC(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_generic_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

#include <pthread.h>

#include <cassert>

namespace Cold::Base {

class CAPABILITY("mutex") Mutex {
 public:
  Mutex() { pthread_mutex_init(&mutex_, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&mutex_); }

  Mutex(const Mutex &) = delete;
  Mutex &operator=(const Mutex &) = delete;

  void Lock() ACQUIRE() { pthread_mutex_lock(&mutex_); }

  void Unlock() RELEASE() { pthread_mutex_unlock(&mutex_); }

  bool Trylock() TRY_ACQUIRE(true) {
    return pthread_mutex_trylock(&mutex_) == 0;
  }

  pthread_mutex_t *NativeHandle() { return &mutex_; }

 private:
  pthread_mutex_t mutex_;
};

class CAPABILITY("spinlock") SpinLock {
 public:
  SpinLock() { pthread_spin_init(&spinlock_, PTHREAD_PROCESS_PRIVATE); }
  ~SpinLock() { pthread_spin_destroy(&spinlock_); }

  SpinLock(const SpinLock &) = delete;
  SpinLock &operator=(const SpinLock &) = delete;

  void Lock() ACQUIRE() { pthread_spin_lock(&spinlock_); }

  void Unlock() RELEASE() { pthread_spin_unlock(&spinlock_); }

  bool Trylock() TRY_ACQUIRE(true) {
    return pthread_spin_trylock(&spinlock_) == 0;
  }

  pthread_spinlock_t *NativeHandle() { return &spinlock_; }

 private:
  pthread_spinlock_t spinlock_;
};

class CAPABILITY("SharedMutex") SharedMutex {
 public:
  SharedMutex() { pthread_rwlock_init(&rwlock_, nullptr); }
  ~SharedMutex() { pthread_rwlock_destroy(&rwlock_); }

  SharedMutex(const SharedMutex &) = delete;
  SharedMutex &operator=(const SharedMutex &) = delete;

  void Lock() ACQUIRE() { pthread_rwlock_wrlock(&rwlock_); }

  void Unlock() RELEASE() { pthread_rwlock_unlock(&rwlock_); }

  bool Trylock() TRY_ACQUIRE(true) {
    return pthread_rwlock_trywrlock(&rwlock_) == 0;
  }

  void ReaderLock() ACQUIRE_SHARED() { pthread_rwlock_rdlock(&rwlock_); }

  void ReaderUnlock() RELEASE_SHARED() { pthread_rwlock_unlock(&rwlock_); }

  bool ReaderTryLock() TRY_ACQUIRE_SHARED(true) {
    return pthread_rwlock_tryrdlock(&rwlock_) == 0;
  }

  pthread_rwlock_t *NativeHandle() { return &rwlock_; }

 private:
  pthread_rwlock_t rwlock_;
};

// // Tag types for selecting a constructor.
struct AdoptLockType {
} inline constexpr AdoptLock = {};
struct DeferLockType {
} inline constexpr DeferLock = {};
struct SharedLockType {
} inline constexpr SharedLock = {};

template <typename T>
requires(requires(T l) {
  l.Lock();
  l.Unlock();
}) class SCOPED_CAPABILITY LockGuard {
 public:
  explicit LockGuard(T &lock) ACQUIRE(lock) : lock_(lock), locked_(true) {
    lock_.Lock();
  };

  ~LockGuard() RELEASE() {
    if (locked_) lock_.Unlock();
  }

  LockGuard(const LockGuard &) = delete;
  LockGuard &operator=(const LockGuard &) = delete;

  LockGuard(T &lock, AdoptLockType) REQUIRES(lock)
      : lock_(lock), locked_(true) {}

  LockGuard(T &lock, AdoptLockType, SharedLockType) REQUIRES_SHARED(lock)
      : lock_(lock), locked_(true) {}

  LockGuard(T &lock, DeferLockType) EXCLUDES(lock)
      : lock_(lock), locked_(false) {}

  void Lock() ACQUIRE() {
    lock_.Lock();
    locked_ = true;
  }

  bool Trylock() TRY_ACQUIRE(true) { return locked_ = lock_.Trylock(); }

  void Unlock() RELEASE() {
    lock_.Unlock();
    locked_ = false;
  }

 private:
  T &lock_;
  bool locked_;
};

class SCOPED_CAPABILITY SharedLockGuard {
 public:
  explicit SharedLockGuard(SharedMutex &lock) ACQUIRE(lock)
      : lock_(lock), locked_(true) {
    lock_.Lock();
  };

  ~SharedLockGuard() RELEASE() {
    if (locked_) lock_.Unlock();
  }

  SharedLockGuard(const SharedLockGuard &) = delete;
  SharedLockGuard &operator=(const SharedLockGuard &) = delete;

  SharedLockGuard(SharedMutex &lock, AdoptLockType) REQUIRES(lock)
      : lock_(lock), locked_(true) {}

  SharedLockGuard(SharedMutex &lock, SharedLockType) ACQUIRE_SHARED(lock)
      : lock_(lock), locked_(true) {
    lock_.ReaderLock();
  }

  SharedLockGuard(SharedMutex &lock, AdoptLockType, SharedLockType)
      REQUIRES_SHARED(lock)
      : lock_(lock), locked_(true) {}

  SharedLockGuard(SharedMutex &lock, DeferLockType) EXCLUDES(lock)
      : lock_(lock), locked_(false) {}

  void Lock() ACQUIRE() {
    lock_.Lock();
    locked_ = true;
  }

  bool Trylock() TRY_ACQUIRE(true) { return locked_ = lock_.Trylock(); }

  void ReaderLock() ACQUIRE_SHARED() {
    lock_.ReaderLock();
    locked_ = true;
  }

  bool ReaderTryLock() TRY_ACQUIRE_SHARED(true) {
    return locked_ = lock_.ReaderTryLock();
  }

  void Unlock() RELEASE() {
    lock_.Unlock();
    locked_ = false;
  }

  void ReaderUnlock() RELEASE() {
    lock_.ReaderUnlock();
    locked_ = false;
  }

 private:
  SharedMutex &lock_;
  bool locked_;
};

#ifdef USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES
// The original version of thread safety analysis the following attribute
// definitions.  These use a lock-based terminology.  They are still in use
// by existing thread safety code, and will continue to be supported.

// Deprecated.
#define PT_GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_var)

// Deprecated.
#define GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE__(guarded_var)

// Replaced by REQUIRES
#define EXCLUSIVE_LOCKS_REQUIRED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(__VA_ARGS__))

// Replaced by REQUIRES_SHARED
#define SHARED_LOCKS_REQUIRED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(shared_locks_required(__VA_ARGS__))

// Replaced by CAPABILITY
#define LOCKABLE THREAD_ANNOTATION_ATTRIBUTE__(lockable)

// Replaced by SCOPED_CAPABILITY
#define SCOPED_LOCKABLE THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

// Replaced by ACQUIRE
#define EXCLUSIVE_LOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_lock_function(__VA_ARGS__))

// Replaced by ACQUIRE_SHARED
#define SHARED_LOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(shared_lock_function(__VA_ARGS__))

// Replaced by RELEASE and RELEASE_SHARED
#define UNLOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(unlock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_trylock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE_SHARED
#define SHARED_TRYLOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(shared_trylock_function(__VA_ARGS__))

// Replaced by ASSERT_CAPABILITY
#define ASSERT_EXCLUSIVE_LOCK(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_exclusive_lock(__VA_ARGS__))

// Replaced by ASSERT_SHARED_CAPABILITY
#define ASSERT_SHARED_LOCK(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_lock(__VA_ARGS__))

// Replaced by EXCLUDE_CAPABILITY.
#define LOCKS_EXCLUDED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

// Replaced by RETURN_CAPABILITY
#define LOCK_RETURNED(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#endif  // USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES

}  // namespace Cold::Base

#endif /* COLD_THREAD_LOCK */
