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


class A
{
public:
	int num;
	std::string text;

	A() :num(5), text("abc") {};
	~A() {};
};

static GMainLoop *gloop = NULL;


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


	gloop = g_main_loop_new(NULL, FALSE);

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

			//api.RequestPtzAbsolutePosition(_p, _t, zoom_f, true, true, false);

			sleep(1);

			LOGINFO("completed");

		}

		LOGINFO("closing...");

		if (gloop)
		{
			g_main_loop_quit(gloop);
		}

		LOGINFO("closed");
	});

	/*
	std::thread t2([&]
	{
		gloop = g_main_loop_new(NULL, FALSE);

		g_main_loop_run(gloop);

		g_main_loop_unref(gloop);
	});

	t1.join();
	t2.join();*/


	/*
	{

		while (true)
		{
			A* a = new A();

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

			delete a;
		}

	}*/

	LOGINFO("loop start");

	g_main_loop_run(gloop);

	LOGINFO("loop end");

	g_main_loop_unref(gloop);

	LOGINFO("loop closed");

	t1.join();

	LOGINFO("thread end");

	sleep(1);

	LOGINFO("end");

	return 0;
}
