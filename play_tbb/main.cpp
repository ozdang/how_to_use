#include <tbb/tbb.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#include <vector>
#include <string>
#include <cstdio>
#include <map>
#include <tbb/concurrent_vector.h>
#include <tbb/task_scheduler_observer.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>

int test_parallel_to_int() {
    printf("test_parallel_to_int\n");

    tbb::parallel_for(
        0, 10, 1,
        [](int& e) {
            printf("%d, ", e);
        }
    );

    printf("\ntest_parallel_to_int end\n");
    return 0;
}

int test_parallel_to_vector_iterator(void) {
    printf("\ntest_parallel_to_vector_iterator\n");

#if 0
    std::vector<int> v(5, 2);
    tbb::parallel_for(
        v.cbegin(), v.cend(), // Index 타입은 std::vector<int>::const_iterator
        [](std::vector<int>::const_iterator& it) {
            printf("%d, ", *it);
        }
    );    
#endif

    printf("\ntest_parallel_to_vector_iterator end\n");
    return 0;    
}


int test_parallel_to_vector_range(void) {
    printf("\ntest_parallel_to_vector_range\n");

    // Range 타입이 그대로 operator()에서 사용되어야 함
    class body_object {
    public:
        void operator()(tbb::blocked_range<std::vector<int>::iterator>& r) const {
            for(const auto& e : r) {
                printf("%d, ", e);
            }
        }
    } my_body;
    std::vector<int> v(5, 3);
    tbb::blocked_range<std::vector<int>::iterator> my_range{v.begin(), v.end()};

    tbb::parallel_for(
        my_range, my_body
    );

    printf("\ntest_parallel_to_vector_range end\n");
    return 0;
}

int test_parallel_to_map_range(void) {
    printf("\ntest_parallel_to_map_range end\n");

#if 0
    class body_object {
    public:
        void operator()(tbb::blocked_range<std::map<int, int>::iterator>& r) const {
            
        }
    } my_body;

    std::map<int, int> m;
    bool op_result = m.begin() < m.end(); // 비교 연산자를 지원하지 않아서 Range의 Value 타입에 대입할 수 없음

    tbb::blocked_range<std::map<int, int>::iterator> my_range{m.begin(), m.end()};
    tbb::parallel_for(
        my_range, my_body
    );    
#endif

    printf("\ntest_parallel_to_map_range end\n");
    return 0;
}

int test_parallel_each_to_vector(void) {
    printf("\ntest_parallel_each_to_vector\n");

    class body_object {
    public:
        void operator()(int r) const {
            printf("%d, ", r);
        }
    } my_body;
    std::vector<int> v(5, 5);
    tbb::parallel_for_each(
        v, my_body
    );

    printf("\ntest_parallel_each_to_vector end\n");

    return 0;
}

int test_parallel_each_to_map(void) {
    printf("\ntest_parallel_each_to_map\n");

    class body_object {
    public:
        void operator()(const std::pair<const int, int>& r) const {
            printf("{%d, %d}, ", r.first, r.second);
        }
    } my_body;
    std::map<int, int> m{{1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}};
    tbb::parallel_for_each(
        m, my_body
    );

    printf("\ntest_parallel_each_to_map end\n");

    return 0;
}

std::string gen_random(const int len) {
    
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) 
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    
    return tmp_s;
    
}

int test_parallel_reduce_for_vector() {
    printf("\ntest_parallel_reduce_for_vector\n");

    class body_object {
    public:
        body_object() = default;

        explicit body_object(body_object& body, tbb::split s) {
            output.clear();
        }

        void operator()(const tbb::blocked_range<std::vector<std::string>::const_iterator>& r) {
            for(const auto& e : r) {
                //printf("%s, ", e.data());
                //output.emplace_back(e + "_" + std::to_string(pthread_self()));
                printf("%lu\n", pthread_self());
            }
        }

        void join(body_object& rhs) {
            output.grow_by(rhs.output.begin(), rhs.output.end());

            /*
            for(const auto& e : rhs.output) {
                output.emplace_back(e);
            }
            */
        }

        tbb::concurrent_vector<std::string> output;
    } my_body;

    srand( (unsigned) time(NULL));
    std::vector<std::string> v;
    for(int i = 0; i < 512*16; i++) {
        v.emplace_back(gen_random(12));
    }
    printf("\nv size : %lu\n", v.size());

    tbb::blocked_range<std::vector<std::string>::const_iterator> my_range{v.begin(), v.end()};

    tbb::parallel_reduce(
        my_range, my_body
    );

    const auto& output = my_body.output;
    printf("\noutput size : %lu\n", output.size());
    std::for_each(output.begin(), output.end(),
        [](const std::string& s) {
            printf("- %s\n", s.data());
        }
    );

    printf("\ntest_parallel_reduce_for_vector end\n");

    return 0;
}

static void parent_sigchld_handler(int sig)
{
    int savedErrno = errno;

    if(sig != SIGCHLD) {
        abort();
    }

    pid_t childPid;
    siginfo_t siginfo{0,};
    // 하나 이상의 좀비 자식 프로세스를 확인하기 위해 반복문을 사용한다.
    // waitid   return 0 : 성공 / WNOHANG 플래그 사용 시 기다리는 자식이 없으면 0
    //          return -1 : 더 이상 자식 프로세스가 없음(ECHILD)
    int waitid_ret = 0;
    int child_sig = 0;
    while ((waitid_ret = waitid(P_ALL, -1, &siginfo, WEXITED | WNOHANG)) == 0) {
        // waitid는 자식 PID를 siginfo_t 구조체에서 확인한다.
        childPid = siginfo.si_pid;
        switch (siginfo.si_code) {
            case CLD_KILLED :
                break;
            case CLD_EXITED :
                break;
            default :
                break;
        }
    }

    if (waitid_ret == -1 && errno != ECHILD) {
        return;
    }

    printf("[MP][HANDLR] Return to wait code"); fflush(nullptr);

    errno = savedErrno;

    return;
}

int test_two_parallel_parent_and_child(void) {
    class body_object {
    public:
        body_object() = default;

        explicit body_object(body_object& body, tbb::split s) {
            output.clear();
        }

        void operator()(const tbb::blocked_range<std::vector<std::string>::const_iterator>& r) {
            for(const auto& e : r) {
                //printf("%s, ", e.data());
                //output.emplace_back(e + "_" + std::to_string(pthread_self()));
                printf("1- %lu\n", pthread_self());
            }
        }

        void join(body_object& rhs) {
            output.grow_by(rhs.output.begin(), rhs.output.end());

            /*
            for(const auto& e : rhs.output) {
                output.emplace_back(e);
            }
            */
        }

        tbb::concurrent_vector<std::string> output;
    } my_body;

    srand( (unsigned) time(NULL));
    std::vector<std::string> v;
    for(int i = 0; i < 512*16; i++) {
        v.emplace_back(gen_random(12));
    }
    printf("\nv size : %lu\n", v.size());

    tbb::blocked_range<std::vector<std::string>::const_iterator> my_range{v.begin(), v.end()};

    tbb::parallel_reduce(
            my_range, my_body
    );

    // 시그널 처리 #1 : 반드시 자식 프로세스 생성 전 SIGCHLD을 받을 핸들러를 설치해야 한다.
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = parent_sigchld_handler;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        return -1;
    }

    // 시그널 처리 #2 : 우선 SIGCHLD 신호 전달을 블록(대기 상태)한다.
    // 이는 sigsuspend 실행 전에 자식 프로세스가 종료되어 SIGCHLD를 전달하는 것을 방지하기 위함이다.
    sigset_t blockMask, emptyMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    if (sigprocmask(SIG_SETMASK, &blockMask, nullptr) == -1) {
        return -2;
    }

    int fork_id = fork();
    if(fork_id == -1) {
        printf("errrroror");
    } else if(fork_id == 0) {
        // 자식 프로세스
        class body_object2 {
        public:
            void operator()(tbb::blocked_range<std::vector<std::string>::iterator>& r) const {
                for(const auto& e : r) {
                    printf("2- %lu\n", pthread_self());
                }
            }
        } my_body2;
        std::vector<std::string> v2;
        for(int i = 0; i < 512*16; i++) {
            v2.emplace_back(gen_random(12));
        }

        tbb::blocked_range<std::vector<std::string>::iterator> my_range2{v2.begin(), v2.end()};

        tbb::parallel_for(
                my_range2, my_body2
        );

        return 0;
    }

    sigemptyset(&emptyMask);
    if (sigsuspend(&emptyMask) == -1 && errno != EINTR) {
        return 0;
    }

    return 0;
}

int main() {
    //tbb::global_control gc(tbb::global_control::max_allowed_parallelism, 10);
    int mp = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);

    printf("TBB THREAD NUM : %d\n", mp);

#if 0
    test_parallel_to_int();
    test_parallel_to_vector_iterator();
    test_parallel_to_vector_range();
    test_parallel_each_to_vector();
    test_parallel_each_to_map();
    test_parallel_reduce_for_vector();
#endif
    test_two_parallel_parent_and_child();

    /*!
    *
    * @note 결론 : Range 범위로 나눠진 그룹을 받고자 할 때는 parallel_for을 사용하고,
    *       만약 body 객체에 변경이 필요하면, parallel_reduce를 사용하자
    *       그리고 Range는 std::map과 std::list의 반복자를 받지 못한다.
    */
   
    return 0;
}