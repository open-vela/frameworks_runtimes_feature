#pragma once

#include <pthread.h>

class ThreadChecker {
public:
    ThreadChecker()
        : pid_ { pthread_self() }
    {
    }

    [[nodiscard]] bool checkThreadValid() const
    {
        return pthread_self() == pid_;
    }

private:
    pthread_t pid_ {};
};

#ifdef CONFIG_FEATURE_ENABLE_THREAD_CHECKER
#define THREAD_CHECK(checker)                                                              \
    if (checker && !((ThreadChecker*)checker)->checkThreadValid()) [[unlikely]] {          \
        FEATURE_LOG_INFO("FATAL: TRY TO CALL JS FUNCTION BUT NOT IN JS THREAD, ABORT!!!"); \
        assert(0);                                                                         \
    }
#else
#define THREAD_CHECK(ignore)
#endif // CONFIG_FEATURE_ENABLE_THREAD_CHECKER
