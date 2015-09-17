#include "ril_event.h"

extern "C" {
extern void RIL_startEventLoop();
extern void RIL_register();
}

static struct ril_event s_listen_event;
static int s_fdListen = -1;



int main(int argc, char *argv[])
{
	// ¿ªÆôEventLoopÑ­»·
	RIL_startEventLoop();
	
	// ×¢²á
	RIL_register();
	
	while(1) {
        // sleep(UINT32_MAX) seems to return immediately on bionic
        sleep(0x00ffffff);
    }
	
	return 0;
}
