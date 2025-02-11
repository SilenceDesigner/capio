#ifndef CAPIO_POSIX_UTILS_CLONE_HPP
#define CAPIO_POSIX_UTILS_CLONE_HPP

#include <condition_variable>
#include <unordered_set>

#include "capio/syscall.hpp"

#include "requests.hpp"

inline std::mutex clone_mutex;
inline std::condition_variable clone_cv;
inline std::unordered_set<pid_t> *tids;

inline bool is_capio_tid(const pid_t tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld)", tid);
    const std::lock_guard<std::mutex> lg(clone_mutex);
    LOG("tis %s a CAPIO tid", tids->find(tid) != tids->end() ? "is" : "is not");
    return tids->find(tid) != tids->end();
}

inline void register_capio_tid(const pid_t tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld)", tid);
    const std::lock_guard<std::mutex> lg(clone_mutex);
    tids->insert(tid);
    LOG("Inserted tid into map");
}

inline void remove_capio_tid(const pid_t tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld)", tid);
    const std::lock_guard<std::mutex> lg(clone_mutex);
    tids->erase(tid);
    LOG("Removed tid from map");
}

inline void init_threading_support() {
    START_LOG(capio_syscall(SYS_gettid), "call()");
    tids = new std::unordered_set<pid_t>{};
}

inline void init_process(pid_t tid) {
    START_LOG(syscall_no_intercept(SYS_gettid), "call(tid=%ld)", tid);

    syscall_no_intercept_flag = true;

    auto name = SHM_COMM_CHAN_NAME_RESP + std::to_string(tid);
    LOG("Allocating new circular buffer with name: %s", name.c_str());
    auto *p_buf_response =
        new CircularBuffer<capio_off64_t>(name, CAPIO_REQ_BUFF_CNT, sizeof(capio_off64_t));
    bufs_response->insert(std::make_pair(tid, p_buf_response));

    LOG("Created request response buffer with name: %s",
        (SHM_COMM_CHAN_NAME_RESP + std::to_string(tid)).c_str());

    const char *capio_app_name = get_capio_app_name();

    /**
     * The previous if, for an anonymous handshake was present, however the get_capio_app_name()
     * never returns a nullptr, as there is a default name, thus rendering the
     * handshake_anonymous_request() useless
     */
    handshake_request(tid, capio_app_name);

    syscall_no_intercept_flag = false;
}

inline void hook_clone_child() {
    auto tid = static_cast<pid_t>(capio_syscall(SYS_gettid));

#ifdef __CAPIO_POSIX
    syscall_no_intercept_flag = true;
#endif
    std::unique_lock<std::mutex> lock(clone_mutex);
    clone_cv.wait(lock, [&tid] { return tids->find(tid) != tids->end(); });

    /**
     * Freeing memory here through `tids.erase()` can cause a SIGSEGV error
     * in the libc, which tries to load the `__ctype_b_loc` table but fails
     * because it is not initialized yet. For this reason, a thread's `tid`
     * is removed from the `tids` set only when the thread terminates.
     */
    lock.unlock();
    START_SYSCALL_LOGGING();
    START_LOG(tid, "call()");
    LOG("Initializing child thread %d", tid);
    init_process(tid);
    LOG("Child thread %d initialized", tid);
    LOG("Starting child thread %d", tid);
    init_caches();
#ifdef __CAPIO_POSIX
    syscall_no_intercept_flag = false;
#endif
}

inline void hook_clone_parent(long child_tid) {

    START_LOG(static_cast<pid_t>(capio_syscall(SYS_gettid)), "call(child_tid=%d)", child_tid);

    if (child_tid < 0) {
        LOG("Skipping clone as child tid is set to %d: %s", child_tid, std::strerror(child_tid));
        return;
    }

    SUSPEND_SYSCALL_LOGGING();
    register_capio_tid(child_tid);
    clone_cv.notify_all();
}

#endif // CAPIO_POSIX_UTILS_CLONE_HPP
