#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <pthread.h>
#include <sys/syscall.h>

#include <boost/timer/timer.hpp>

#include <tbb/tbb.h>

tbb::concurrent_bounded_queue<int> queue;

void f1()
{
#ifdef __linux__
    pid_t tid = syscall(SYS_gettid);
    std::cout << "Tid of job f1: " << tid<< std::endl;
#endif

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Job f1 done ..." << std::endl;
};

void f2()
{
#ifdef __linux__
    pid_t tid = syscall(SYS_gettid);
    std::cout << "Tid of job f2: " << tid<< std::endl;
#endif

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Job f2 done ..." << std::endl;
};

void f3(const tbb::blocked_range<size_t>& r)
{
#ifdef __linux__
    pid_t tid = syscall(SYS_gettid);
    std::cout << "Tid of job f3: " << tid<< std::endl;
#endif

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << r.begin() << "/"<< r.end() << std::endl;
    std::cout << "Job done ..." << std::endl;
};

void f4()
{
#ifdef __linux__
    pid_t tid = syscall(SYS_gettid);
    std::cout << "Tid of job f1: " << tid<< std::endl;
#endif

    int i;
    queue.pop(i);
    std::cout << "Job f4 done, got: " << i << std::endl;
};

void f5()
{
#ifdef __linux__
    pid_t tid = syscall(SYS_gettid);
    std::cout << "Tid of job f2: " << tid<< std::endl;
#endif

    int i;
    queue.pop(i);
    std::cout << "Job f5 done, got: " << i << std::endl;
};
struct Observer : tbb::task_scheduler_observer
{
    int _rt_prio;

    Observer(bool b = true, int rt_prio = 30) : _rt_prio(rt_prio) { observe(b); }

    void on_scheduler_entry(bool)
    {
#ifdef __linux__
        pthread_t pid = pthread_self();
        pid_t tid = syscall(SYS_gettid);

        //The thread name is a meaningful C language string, whose length is
        //restricted to 16 characters, including the terminating null byte ('\0')
        std::string s = "TBB-" + std::to_string(tid);
        std::cout << s << std::endl;

        if(pthread_setname_np(pid, s.data()))
            std::cout << "Could not set tbb names" << std::endl;

        struct sched_param param = {};
        param.sched_priority = _rt_prio;

        if(pthread_setschedparam(pid, SCHED_FIFO, &param))
            std::cout << "Could not set realtime parameter" << std::endl;
#endif
    }

    void on_scheduler_exit(bool) { std::cout << "Off:" << pthread_self() << std::endl; }
};

constexpr int loop_cnt = 1;
constexpr int max_threads = 4;

int main(void)
{
    Observer observer;
    tbb::task_scheduler_init init(max_threads);
    tbb::task_group tg;

    std::cout << "TBB threads, max available: " << init.default_num_threads() << std::endl;

    std::function<void(const tbb::blocked_range<size_t>& r)> f = f3;

    {
        boost::timer::auto_cpu_timer act;

        for(int i = 0; i < loop_cnt; i++)
        {
            tbb::parallel_for(tbb::blocked_range<size_t>(0, 16, 4), f, tbb::simple_partitioner());

            std::cout << "All _for_ jobs done ..." << std::endl;

            tbb::parallel_invoke(f1, f2);

            std::cout << "All _invoke_ jobs done ..." << std::endl;

            tg.run([](){f4();});
            tg.run([](){f5();});

            std::this_thread::sleep_for(std::chrono::seconds(5));

            queue.push(-1);
            queue.push(-2);

            tg.wait();

            std::cout << "All _taskgroup_ jobs done ..." << std::endl;
        }
    }

    exit(0);
}
