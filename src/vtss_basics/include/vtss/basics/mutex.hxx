/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.

*/

#ifndef _VTSS_MUTEX_H_
#define _VTSS_MUTEX_H_

#include <pthread.h>
#include <string>
#include "vtss/basics/assert.hxx"
#include "vtss/basics/module_id.hxx"

#if defined(VTSS_BASICS_STANDALONE)
#include <mutex>
#else
#include "critd_api.h"
#endif

namespace vtss {
#if defined(VTSS_BASICS_STANDALONE)
class mutex {
    pthread_mutex_t m;

    mutex(const mutex &) = delete;
    mutex &operator=(const mutex &) = delete;

  public:
    inline constexpr mutex() : m(PTHREAD_MUTEX_INITIALIZER) {}
    ~mutex() { pthread_mutex_destroy(&m); }

    void lock() {
        int ec = pthread_mutex_lock(&m);
        VTSS_ASSERT(ec == 0);
    }

    bool try_lock() { return pthread_mutex_trylock(&m) == 0; }

    void unlock() {
        int ec = pthread_mutex_unlock(&m);
        VTSS_ASSERT(ec == 0);
    }

    typedef pthread_mutex_t *native_handle_type;
    inline native_handle_type native_handle() { return &m; }
};

class Critd {
  private:
    std::mutex m;

  public:
    Critd(const char *name, uint32_t module_id = VTSS_MODULE_ID_BASICS) {}
    void lock(const char *f, unsigned l) { m.lock(); }
    void unlock(const char *f, unsigned l) { m.unlock(); }

    int critd_prepare_wait() { return 0; }
    void critd_done_wait(int i) { }
};

class CritdRecursive {
  private:
    std::recursive_mutex m;

  public:
    CritdRecursive(const char *name,
                   uint32_t module_id = VTSS_MODULE_ID_BASICS,
                   bool leaf = false) {}
    void lock(const char *f, unsigned l) { m.lock(); }
    void unlock(const char *f, unsigned l) { m.unlock(); }
};

typedef CritdRecursive CritdRecursiveLazyInit;
#else
struct CritdFileAndLine {
    const char *file;
    int line;
};

class Critd {
  private:
    critd_t critd_;

  public:
    Critd(const char *name, uint32_t module_id = VTSS_MODULE_ID_BASICS) {
        ::critd_init(&critd_, name, module_id, CRITD_TYPE_MUTEX);
    }

    pthread_mutex_t *native_handle() { return &critd_.m.mutex; }

    ~Critd() { ::critd_delete(&critd_); }

    const CritdFileAndLine critd_prepare_wait() {
        CritdFileAndLine where = {critd_.lock_file, critd_.lock_line};
        ::critd_exit(&critd_, __FILE__, __LINE__, true);
        return where;
    }

    void critd_done_wait(const CritdFileAndLine &where) {
        ::critd_enter(&critd_, where.file, where.line, true);
    }

    void lock(const char *f, unsigned l) {
        ::critd_enter(&critd_, f, l);
    }

    void unlock(const char *f, unsigned l) {
        ::critd_exit(&critd_, f, l);
    }
};

class CritdRecursive {
  private:
    critd_t critd_;

  public:
    CritdRecursive(const char *name,
                   uint32_t module_id = VTSS_MODULE_ID_BASICS) {
        ::critd_init(&critd_, name, module_id, CRITD_TYPE_MUTEX_RECURSIVE);
    }

    ~CritdRecursive() { ::critd_delete(&critd_); }

    void lock(const char *f, unsigned l) {
        ::critd_enter(&critd_, f, l);
    }

    void unlock(const char *f, unsigned l) {
        ::critd_exit(&critd_, f, l);
    }
};

// TODO, fix the initialization problems in Critd and get rid of this crap...
class CritdRecursiveLazyInit {
  private:
    critd_t critd_;
    std::string name_;
    uint32_t module_id_;
    bool leaf_;

  public:
    CritdRecursiveLazyInit(const char *name,
                           uint32_t   module_id = VTSS_MODULE_ID_BASICS,
                           bool       leaf = false)
        : name_(name), module_id_(module_id), leaf_(leaf) {}

    ~CritdRecursiveLazyInit() {
        if (critd_.init_done) {
            ::critd_delete(&critd_);
        }
    }

    void lock(const char *f, unsigned l);

    void unlock(const char *f, unsigned l) {
        ::critd_exit(&critd_, f, l);
    }
};
#endif

struct defer_lock_t {};
struct try_to_lock_t {};
struct adopt_lock_t {};

template <typename Mutex = Critd>
class lock_guard {
  private:
    Mutex &m;
    explicit lock_guard(lock_guard &);
    lock_guard &operator=(lock_guard &);

  public:
    explicit lock_guard(const char *f, unsigned l, Mutex &m_) : m(m_) {
        m.lock(f, l);
    }
    lock_guard(Mutex &m_, adopt_lock_t) : m(m_) {}
    ~lock_guard() { m.unlock(__FILE__, __LINE__); }
};

template <typename Mutex>
class unique_lock {
  private:
    Mutex *m;
    bool is_locked;
    unique_lock(unique_lock &);
    unique_lock &operator=(unique_lock &);

  public:
    unique_lock() : m(0), is_locked(false) {}

    explicit unique_lock(const char *f, unsigned l, Mutex &m_)
        : m(&m_), is_locked(false) {
        lock(f, l);
    }
    unique_lock(Mutex &m_, adopt_lock_t) : m(&m_), is_locked(true) {}
    unique_lock(Mutex &m_, defer_lock_t) : m(&m_), is_locked(false) {}
    unique_lock(Mutex &m_, try_to_lock_t) : m(&m_), is_locked(false) {
        try_lock();
    }

    ~unique_lock() {
        if (owns_lock()) m->unlock(__FILE__, __LINE__);
    }

    void lock(const char *f, unsigned l) {
        VTSS_ASSERT(!owns_lock());
        m->lock(f, l);
        is_locked = true;
    }

    bool try_lock() {
        is_locked = m->try_lock();
        return is_locked;
    }

    void unlock() {
        VTSS_ASSERT(owns_lock());
        m->unlock(__FILE__, __LINE__);
        is_locked = false;
    }
    typedef void (unique_lock::*bool_type)();

    operator bool_type() const { return is_locked ? &unique_lock::lock : 0; }
    bool operator!() const { return !owns_lock(); }
    bool owns_lock() const { return is_locked; }
    Mutex *mutex() const { return m; }

    Mutex *release() {
        Mutex *const res = m;
        m = 0;
        is_locked = false;
        return res;
    }
};

template <typename Mutex>
class condition_variable {
    pthread_cond_t cv;

  public:
    constexpr condition_variable() : cv(PTHREAD_COND_INITIALIZER) {}
    ~condition_variable() { pthread_cond_destroy(&cv); }

  private:
    condition_variable(const condition_variable &) = delete;
    condition_variable &operator=(const condition_variable &) = delete;

  public:
    void notify_one() { pthread_cond_signal(&cv); }
    void notify_all() { pthread_cond_broadcast(&cv); }

    void wait(unique_lock<Mutex> &lock) {
        VTSS_ASSERT(lock.owns_lock());
        auto x = lock.mutex()->critd_prepare_wait();
        int e = pthread_cond_wait(&cv, lock.mutex()->native_handle());
        lock.mutex()->critd_done_wait(x);
        VTSS_ASSERT(e == 0);
    }

    typedef pthread_cond_t *native_handle_type;
    native_handle_type native_handle() { return &cv; }
};

template <typename T>
struct ScopeLock {
    ScopeLock(T *cxt, const char *f, unsigned line)
        : l(cxt), file_(f), line_(line) {
        l->lock(f, line);
    }
    ~ScopeLock() { l->unlock(file_, line_); }
    T *l;
    const char *file_;
    unsigned line_;
};

template <typename T>
struct ScopeUnlock {
    ScopeUnlock(T *cxt, const char *f, unsigned line)
        : l(cxt), file_(f), line_(line) {
        l->unlock(f, line);
    }
    ~ScopeUnlock() { l->lock(file_, line_); }
    T *l;
    const char *file_;
    unsigned line_;
};

}  // namespace vtss
#endif /* _VTSS_MUTEX_H_ */
