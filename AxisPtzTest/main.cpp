#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fixmath.h>
#include <axsdk/axptz.h>
#include "AxisPtzApi.h"
#include <iostream>
#include <thread>


#define LOGINFO(fmt, args...) \
  do { \
    printf(fmt, ## args); \
    printf("\n"); \
  } while(0)


#define LOOP_ENABLED

#ifdef LOOP_ENABLED
static GMainLoop *gloop = NULL;
#endif

/*
* Main
*/
int main(int argc, char **argv)
{
	float zoom_f = 0.5;
	float speed = 0.6f;

	if (argc <= 1)
	{
		return 1;
	}

#ifdef LOOP_ENABLED
	gloop = g_main_loop_new(NULL, FALSE);
#endif

	AxisPtzTest::AxisPtzApi api;

	api.SetMoveSpeed(speed);

	std::thread t1([&]
	{
		while (true)
		{

			float _p, _t;

			std::cout << "pan" << std::endl;
			std::cin >> _p;

			std::cout << "tilt" << std::endl;
			std::cin >> _t;

			if (_p > 1 || _t > 1)
			{
				break;
			}

			LOGINFO("(pan, tilt, zoom) = (%f, %f, %f)", _p, _t, zoom_f);

			api.RequestPtzAbsolutePosition(_p, _t, zoom_f, true, true, false);

			sleep(1);

			LOGINFO("completed");

		}

		LOGINFO("closing...");
#ifdef LOOP_ENABLED
		if (gloop)
		{
			g_main_loop_quit(gloop);
		}
#endif

		LOGINFO("closed");
	});

#ifdef LOOP_ENABLED
	LOGINFO("loop start");

	g_main_loop_run(gloop);

	LOGINFO("loop end");

	g_main_loop_unref(gloop);

	LOGINFO("loop closed");
#endif

	t1.join();

	LOGINFO("thread end");

	sleep(1);

	LOGINFO("end");

	return 0;
}
