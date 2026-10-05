// win32_sem_compat.h
// Windows compatibility shim for POSIX unnamed semaphores (sem_t).
//
// Background: MSVC provides no <semaphore.h>. The offline DSP thread pool
// and the streaming butler thread only need process-private unnamed
// semaphores with initial value 0 (init / wait / post / destroy), which map
// directly onto Win32 semaphore objects.
//
// Usage: include this header INSTEAD of <semaphore.h> inside an
// `#elif defined(_WIN32)` branch, e.g.:
//
//   #ifdef __APPLE__
//   #include <dispatch/dispatch.h>
//   #elif defined(_WIN32)
//   #include "common/win32_sem_compat.h"
//   #else
//   #include <semaphore.h>
//   #endif
//
// Include directories note: top-level CMake adds src/ to the include path,
// so the "common/..." form resolves from any layer.

#pragma once

#if !defined(_WIN32)
#error "win32_sem_compat.h is Windows-only; use <semaphore.h> (POSIX) or dispatch (macOS)"
#endif

#include <climits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Win32-backed stand-in for POSIX sem_t (unnamed, process-private use only).
struct Win32Sem {
    HANDLE handle = nullptr;
};
using sem_t = Win32Sem;

inline int sem_init(sem_t* sem, int /*pshared*/, unsigned int value) {
    if (sem == nullptr) return -1;
    sem->handle = ::CreateSemaphoreA(nullptr, static_cast<LONG>(value), LONG_MAX, nullptr);
    return (sem->handle != nullptr) ? 0 : -1;
}

inline int sem_wait(sem_t* sem) {
    if (sem == nullptr || sem->handle == nullptr) return -1;
    return (::WaitForSingleObject(sem->handle, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

inline int sem_post(sem_t* sem) {
    if (sem == nullptr || sem->handle == nullptr) return -1;
    return ::ReleaseSemaphore(sem->handle, 1, nullptr) ? 0 : -1;
}

inline int sem_destroy(sem_t* sem) {
    if (sem == nullptr) return -1;
    if (sem->handle != nullptr) {
        ::CloseHandle(sem->handle);
        sem->handle = nullptr;
    }
    return 0;
}
