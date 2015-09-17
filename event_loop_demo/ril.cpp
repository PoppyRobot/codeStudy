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

	//Event�ĳ�ʼ��
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

	//����һ��Event
    ril_event_set (&s_wakeupfd_event, s_fdWakeupRead, true,
                processWakeupCallback, NULL);

	//�����洴����Event���뵽watch_table�У������𵽻���LoopEvent�����ã�������ϸ˵��
    rilEventAddWakeup (&s_wakeupfd_event);

	//����loopѭ�� 
    // Only returns on error
    ril_event_loop();

    RLOGE ("error in event_loop_base errno:%d", errno);
    // kill self to restart on error
    //kill(0, SIGKILL);

    return NULL;
}

// ����RILJ�й�����Event������ע��Ļص�������listenCallback��
static void listenCallback (int fd, short flags, void *param) {
	RLOGE ("listenCallback running\n");
}

// ��reference�еĻص�����ע���RIL�Ŀ��
extern "C" void
RIL_register () {
	// �����Socketͨ���������һ��Event
	/* note: non-persistent so we can accept only one connection at a time */
	s_fdListen = open("Makefile", O_RDONLY);
	ril_event_set (&s_listen_event, s_fdListen, false,
				listenCallback, NULL);

	// ��ӵ�Eventloop��
	rilEventAddWakeup (&s_listen_event);
	
	char buffer[10];
	while (read(s_fdListen, buffer, sizeof(buffer)) != 0)
	{
		ril_event_set (&s_listen_event, s_fdListen, false, listenCallback, NULL);

		// ��ӵ�Eventloop��
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

	// ��Event�̣߳�������eventLoop����ѭ��
    ret = pthread_create(&s_tid_dispatch, &attr, eventLoop, NULL);

	// �������һ���߳�Ϊ�����̣߳�������߳������ַǳ��죬���ܿ�����pthread_create��������֮ǰ����ֹ�ˣ�
	// ����ֹ�Ժ�Ϳ��ܽ��̺߳ź�ϵͳ��Դ�ƽ����������߳�ʹ�ã���������pthread_create���߳̾͵õ��˴�����̺߳š�
	// Ҫ��������������Բ�ȡһ����ͬ����ʩ����򵥵ķ���֮һ�ǿ����ڱ��������߳������pthread_cond_timewait������
	// ������̵߳ȴ�һ����������㹻��ʱ���ú���pthread_create���ء�����һ�εȴ�ʱ�䣬���ڶ��̱߳���ﳣ�õķ�����
	// ����ע�ⲻҪʹ������wait()֮��ĺ�����������ʹ��������˯�ߣ������ܽ���߳�ͬ�������⡣
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
