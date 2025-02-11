#ifndef HANDSHAKE_HPP
#define HANDSHAKE_HPP

#include "capio/constants.hpp"

#include <storage-service/capio_storage_service.hpp>

/**
 * @brief Perform handshake while providing the posix application name
 *
 * @param str raw request as read from the shared memory interface stripped of the request number
 * (first parameter of the request)
 */
inline void handshake_handler(const char *const str) {
    pid_t pid;
    char app_name[1024];
    sscanf(str, "%d %s", &pid, app_name);
    START_LOG(gettid(), "call(tid=%ld, app_name=%s)", pid, app_name);
    client_manager->register_client(app_name, pid);
    storage_service->register_client(app_name, pid);
}

#endif // HANDSHAKE_HPP
