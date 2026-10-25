#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define main disabled_main
#include "test_dungeon_run_policy.cpp"
#undef main

int main()
{
    struct T
    {
        const char* name;
        void (*fn)();
    };
    T tests[] = {
        {"TestStandPullWaitsAtTheStandOutOfShot", TestStandPullWaitsAtTheStandOutOfShot},
        {"TestStandPullPullsFromTheStandInShot", TestStandPullPullsFromTheStandInShot},
        {"TestStandPullWaitsWhileOutOfSight", TestStandPullWaitsWhileOutOfSight},
        {"TestStandPullWalksBackToTheStand", TestStandPullWalksBackToTheStand},
        {"TestStandPullCeilingSkipsWithTheSkipFlag", TestStandPullCeilingSkipsWithTheSkipFlag},
        {"TestStandPullNoRangedPull", TestStandPullNoRangedPull},
        {"TestStandPullWithoutStandPullsAsBefore", TestStandPullWithoutStandPullsAsBefore},
    };
    for (const auto& t : tests)
    {
        ::fflush(stdout);
        ::fflush(stderr);
        pid_t p = fork();
        if (p == 0)
        {
            t.fn();
            _exit(0);
        }
        int st = 0;
        waitpid(p, &st, 0);
        bool pass = WIFEXITED(st) && WEXITSTATUS(st) == 0;
        std::printf("%s: %s\n", t.name, pass ? "PASS" : "FAIL");
    }
    return 0;
}
