#include "Log.h"
#include <pthread.h>

#include <sys/types.h>
#include <pwd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <alloca.h>
#include <sys/un.h>
#include <assert.h>
#include <netinet/in.h>
#include "ril_event.h"

static pthread_t s_tid_dispatch;
static pthread_t s_tid_reader;
static int s_started = 0;

static int s_fdListen = -1;
static int s_fdCommand = -1;
static int s_fdDebug = -1;

static int s_fdWakeupRead;
static int s_fdWakeupWrite;

static struct ril_event s_commands_event;
static struct ril_event s_wakeupfd_event;
static struct ril_event s_listen_event;
static struct ril_event s_wake_timeout_event;
static struct ril_event s_debug_event;

static const struct timeval TIMEVAL_WAKE_TIMEOUT = {1,0};

static pthread_mutex_t s_pendingRequestsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_startupMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_startupCond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t s_dispatchMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_dispatchCond = PTHREAD_COND_INITIALIZER;

/**
 * A write on the wakeup fd is done just to pop us out of select()
 * We empty the buffer here and then ril_event will reset the timers on the
 * way back down
 */
static void processWakeupCallback(int fd, short flags, void *param) {
    char buff[16];
    int ret;

    RLOGE("processWakeupCallback");

    /* empty our wakeup socket out */
    do {
        ret = read(s_fdWakeupRead, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}

static void triggerEvLoop() {
    int ret;
    if (!pthread_equal(pthread_self(), s_tid_dispatch)) {
        /* trigger event loop to wakeup. No reason to do this,
         * if we're in the event loop thread */
         do {
            ret = write (s_fdWakeupWrite, " ", 1);
         } while (ret < 0 && errno == EINTR);
    }
}

static void rilEventAddWakeup(struct ril_event *ev) {
    ril_event_add(ev);
    triggerEvLoop();
}

static void *
eventLoop(void *param) {
	RLOGE("eventLoop\n");	
	int ret;
    int filedes[2];

	//Event的初始化
    ril_event_init();

    pthread_mutex_lock(&s_startupMutex);

    s_started = 1;
    pthread_cond_broadcast(&s_startupCond);

    pthread_mutex_unlock(&s_startupMutex);

    ret = pipe(filedes);

    if (ret < 0) {
        RLOGE("Error in pipe() errno:%d", errno);
        return NULL;
    }

    s_fdWakeupRead = filedes[0];
    s_fdWakeupWrite = filedes[1];

    fcntl(s_fdWakeupRead, F_SETFL, O_NONBLOCK);

	//创建一个Event
    ril_event_set (&s_wakeupfd_event, s_fdWakeupRead, true,
                processWakeupCallback, NULL);

	//将上面创建的Event加入到watch_table中，这里起到唤醒LoopEvent的作用，后面详细说明
    rilEventAddWakeup (&s_wakeupfd_event);

	//进入loop循环 
    // Only returns on error
    ril_event_loop();

    RLOGE ("error in event_loop_base errno:%d", errno);
    // kill self to restart on error
    //kill(0, SIGKILL);

    return NULL;
}

// 对于RILJ中过来的Event，我们注册的回调函数是listenCallback：
static void listenCallback (int fd, short flags, void *param) {
	RLOGE ("listenCallback running\n");
}

// 将reference中的回调函数注册给RIL的框架
extern "C" void
RIL_register () {
	// 用这个Socket通道句柄创建一个Event
	/* note: non-persistent so we can accept only one connection at a time */
	s_fdListen = open("Makefile", O_RDONLY);
	ril_event_set (&s_listen_event, s_fdListen, false,
				listenCallback, NULL);

	// 添加到Eventloop中
	rilEventAddWakeup (&s_listen_event);
	
	char buffer[10];
	while (read(s_fdListen, buffer, sizeof(buffer)) != 0)
	{
		ril_event_set (&s_listen_event, s_fdListen, false, listenCallback, NULL);

		// 添加到Eventloop中
		rilEventAddWakeup (&s_listen_event);
		printf("Read: %s\n", buffer);
		sleep(2);
	}
	close(s_fdListen);
}

extern "C" void
RIL_startEventLoop(void) {
	RLOGE("RIL_startEventLoop\n");	
	int ret;
    pthread_attr_t attr;

    /* spin up eventLoop thread and wait for it to get started */
    s_started = 0;
    pthread_mutex_lock(&s_startupMutex);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// 打开Event线程，并调用eventLoop进入循环
    ret = pthread_create(&s_tid_dispatch, &attr, eventLoop, NULL);

	// 如果设置一个线程为分离线程，而这个线程运行又非常快，它很可能在pthread_create函数返回之前就终止了，
	// 它终止以后就可能将线程号和系统资源移交给其他的线程使用，这样调用pthread_create的线程就得到了错误的线程号。
	// 要避免这种情况可以采取一定的同步措施，最简单的方法之一是可以在被创建的线程里调用pthread_cond_timewait函数，
	// 让这个线程等待一会儿，留出足够的时间让函数pthread_create返回。设置一段等待时间，是在多线程编程里常用的方法。
	// 但是注意不要使用诸如wait()之类的函数，它们是使整个进程睡眠，并不能解决线程同步的问题。
    while (s_started == 0) {
        pthread_cond_wait(&s_startupCond, &s_startupMutex);
    }

    pthread_mutex_unlock(&s_startupMutex);
	
	RLOGE("RIL_startEventLoop pthread_mutex_unlock\n");

    if (ret < 0) {
        RLOGE("Failed to create dispatch thread errno:%d", errno);
        return;
    }
}
