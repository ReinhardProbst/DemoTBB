#include <iostream>
#include <string>
#include <cstdio>

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <tbb/tbb.h>

#include <boost/thread/thread.hpp>
#include <boost/timer/timer.hpp>

void foo(const tbb::blocked_range<size_t>& r)
{
    boost::this_thread::sleep_for(boost::chrono::seconds(30));
    std::cout << r.begin() << "/"<< r.end() << std::endl;
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

constexpr int loop_cnt = 1000000;

int main(void)
{
#if 0


    std::string str;
    int result;

    {
        boost::timer::auto_cpu_timer act;

        for(int i = 0; i < LOOP_CNT; i++)
        {
            str = std::to_string(i);
        }
    }
#endif

    Observer observer;
    tbb::task_scheduler_init init;

    std::cout << "TBB threads created: " << init.default_num_threads() << std::endl;

    std::function<void(const tbb::blocked_range<size_t>& r)> f = foo;

    tbb::parallel_for(tbb::blocked_range<size_t>(0,16), f, tbb::auto_partitioner());

    std::cout << "Job done ..." << std::endl;

    boost::this_thread::sleep_for(boost::chrono::seconds(30));

    exit(0);
}
