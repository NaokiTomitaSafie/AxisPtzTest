
#define __STDC_LIMIT_MACROS

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fixmath.h>
#include <axsdk/axptz.h>
#include "AxisPtzApi.h"
#include <iostream>


#define APP_NAME "AxisPtzTest"

#define	LOG_INFO	6	/* informational */

//    syslog(LOG_INFO, fmt, ## args); \

#define LOGINFO(fmt, args...) \
  do { \
    printf(fmt, ## args); \
    printf("\n"); \
  } while(0)

/* The number of fractional bits used in fix-point variables */
#define FIXMATH_FRAC_BITS 16

// Area check
//#define AREA_CHECK

/* global variables */
static AXPTZControlQueueGroup *ax_ptz_control_queue_group = NULL;
static gint video_channel = 1;
static GList *capabilities = NULL;
static AXPTZStatus *unitless_status = NULL;
static AXPTZLimits *unitless_limits = NULL;
static AXPTZLimits *unit_limits = NULL;


static fixed_t fx_one = fx_itox(1, FIXMATH_FRAC_BITS);

class A
{
public:
	int num;
	std::string text;

	A() :num(5), text("abc") {};
	~A() {};
};


static fixed_t convertNormalizedToUnitless(float source, fixed_t max, fixed_t min)
{
	fixed_t gain = fx_ftox(source, FIXMATH_FRAC_BITS);
	return fx_addx(
		fx_mulx(max, gain, FIXMATH_FRAC_BITS),
		fx_mulx(min, fx_subx(fx_one, gain), FIXMATH_FRAC_BITS));
}

#ifdef AREA_CHECK

static float convertUnitlessToNormalized(fixed_t source, fixed_t max, fixed_t min)
{
	fixed_t gain = fx_divx(fx_subx(source, min), fx_subx(max, min), FIXMATH_FRAC_BITS);
	return fx_xtof(gain, FIXMATH_FRAC_BITS);
}
#endif

/*
* Get the camera's supported PTZ move capabilities
*/
static gboolean get_ptz_move_capabilities(void)
{
	GError *local_error = NULL;

	if (!(capabilities =
		ax_ptz_movement_handler_get_move_capabilities(video_channel,
			&local_error)))
	{
		g_error_free(local_error);
		return FALSE;
	}

	return TRUE;
}

/*
* Check if a capability is supported
*/
static gboolean is_capability_supported(const char *capability)
{
	gboolean is_supported = FALSE;
	GList *it = NULL;

	if (capabilities)
	{
		for (it = g_list_first(capabilities); it != NULL; it = g_list_next(it))
		{
			if (!(g_strcmp0((gchar *)it->data, capability)))
			{
				is_supported = TRUE;
				break;
			}
		}
	}

	return is_supported;
}

#if 0
/*
* Wait for the camera to reach it's position
*/
static gboolean wait_for_camera_movement_to_finish(void)
{
	gboolean is_moving = TRUE;
	gushort timer = 0;
	gushort timeout = 10;
	gushort sleep_time = 1;
	GError *local_error = NULL;

	/* Check if camera is moving */
	if (!(ax_ptz_movement_handler_is_ptz_moving
	(video_channel, &is_moving, &local_error)))
	{
		g_error_free(local_error);
		return FALSE;
	}
	sleep(sleep_time);

	/* Wait until camera is in position or until we get a timeout */
	while (is_moving && (timer < timeout))
	{
		if (!(ax_ptz_movement_handler_is_ptz_moving
		(video_channel, &is_moving, &local_error)))
		{
			g_error_free(local_error);
			return FALSE;
		}
		LOGINFO("%d", timer);

		sleep(sleep_time);
		timer++;
	}

	if (is_moving)
	{
		/* Camera is still moving */
		return FALSE;
	}
	else
	{
		LOGINFO("end %d", timer);
		/* Camera is in position */
		return TRUE;
	}
}
#endif

/*
* Perform camera movement to absolute position
*/
static gboolean
move_to_absolute_position(fixed_t pan_value,
	fixed_t tilt_value,
	AXPTZMovementPanTiltSpace pan_tilt_space,
	gfloat speed,
	AXPTZMovementPanTiltSpeedSpace pan_tilt_speed_space,
	fixed_t zoom_value, AXPTZMovementZoomSpace zoom_space)
{
	AXPTZAbsoluteMovement *abs_movement = NULL;
	GError *local_error = NULL;

	/* Set the unit spaces for an absolute movement */
	if ((ax_ptz_movement_handler_set_absolute_spaces
	(pan_tilt_space, pan_tilt_speed_space, zoom_space, &local_error)))
	{
		/* Create an absolute movement structure */
		if ((abs_movement = ax_ptz_absolute_movement_create(&local_error)))
		{

			/* Set the pan, tilt and zoom values for the absolute movement */
			if (!(ax_ptz_absolute_movement_set_pan_tilt_zoom(abs_movement,
				pan_value,
				tilt_value,
				fx_ftox(speed,
					FIXMATH_FRAC_BITS),
				zoom_value,
				AX_PTZ_MOVEMENT_NO_VALUE,
				&local_error)))
			{
				ax_ptz_absolute_movement_destroy(abs_movement, NULL);
				g_error_free(local_error);
				return FALSE;
			}

			/* Perform the absolute movement */
			if (!(ax_ptz_movement_handler_absolute_move(ax_ptz_control_queue_group,
				video_channel,
				abs_movement,
				AX_PTZ_INVOKE_ASYNC, NULL,
				NULL, &local_error)))
			{
				ax_ptz_absolute_movement_destroy(abs_movement, NULL);
				g_error_free(local_error);
				return FALSE;
			}

			/* Now we don't need the absolute movement structure anymore, destroy it */
			if (!(ax_ptz_absolute_movement_destroy(abs_movement, &local_error)))
			{
				g_error_free(local_error);
				return FALSE;
			}
		}
		else
		{
			g_error_free(local_error);
			return FALSE;
		}
	}
	else
	{
		g_error_free(local_error);
		return FALSE;
	}

	return TRUE;
}



/*
* Main
*/
int main(int argc, char **argv)
{
	GList *it = NULL;
	GError *local_error = NULL;

	bool panEnabled = false;
	bool tiltEnabled = false;
	bool zoomEnabled = false;

	// normalized space
	float pan_f = 0, tilt_f = 0, zoom_f = 0;

	// unitless space
	fixed_t pan_use, tilt_use, zoom_use;

	float speed = 0.6f;

#ifdef WRITE_TO_SYS_LOG
	openlog(APP_NAME, LOG_PID | LOG_CONS, LOG_USER);
#endif
	//LOGINFO("\'axptz library\' example application started...\n");


	// analyze arguments
	if (argc > 1)
	{
		int i;
		for (i = 0; i < argc - 1; i++)
		{
			if (strcmp(argv[i], "-p") == 0)
			{
				panEnabled = true;
				pan_f = atof(argv[++i]);
				//LOGINFO("pan = %f\n", pan_f);
			}
			else if (strcmp(argv[i], "-t") == 0)
			{
				tiltEnabled = true;
				tilt_f = atof(argv[++i]);
				//LOGINFO("tilt = %f\n", tilt_f);
			}
			else if (strcmp(argv[i], "-z") == 0)
			{
				zoomEnabled = true;
				zoom_f = atof(argv[++i]);
				//LOGINFO("zoom = %f\n", zoom_f);
			}
			else if (strcmp(argv[i], "-s") == 0)
			{
				speed = atof(argv[++i]);
				//LOGINFO("speed = %f\n", speed);
			}
		}
	}
	else
	{
		return 1;
	}

	{
		AxisPtzTest::AxisPtzApi api;

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
			LOGINFO("speed = %f", speed);

			api.RequestPtzAbsolutePosition(_p, _t, zoom_f, panEnabled, tiltEnabled, zoomEnabled);

			sleep(1);

			LOGINFO("completed");

			delete a;
		}

		return 0;
	}


	/* Create the axptz library */
	if (!(ax_ptz_create(&local_error)))
	{
		goto failure;
	}

	/* Get the application group from the PTZ control queue */
	if (!(ax_ptz_control_queue_group =
		ax_ptz_control_queue_get_app_group_instance(&local_error)))
	{
		goto failure;
	}

	/* Get the supported capabilities */
	if (!(get_ptz_move_capabilities()))
	{
		goto failure;
	}

	/* Get the current status (e.g. the current pan/tilt/zoom value/position) */
	if (!(ax_ptz_movement_handler_get_ptz_status(video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&unitless_status,
		&local_error)))
	{
		goto failure;
	}

	//LOGINFO("Now we got the current PTZ status.\n");

	/* Get the pan, tilt and zoom limits for the unitless space */
	if (!(ax_ptz_movement_handler_get_ptz_limits(video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&unitless_limits,
		&local_error)))
	{
		goto failure;
	}

	/* Get the pan, tilt and zoom limits for the unit (degrees) space */
	if (!(ax_ptz_movement_handler_get_ptz_limits(video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_DEGREE,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&unit_limits, &local_error)))
	{
		goto failure;
	}

	//LOGINFO("Now we got the current PTZ limits.\n");



	// convert parameters
	if (!panEnabled || !is_capability_supported("AX_PTZ_MOVE_ABS_PAN"))
	{
		pan_use = AX_PTZ_MOVEMENT_NO_VALUE;
	}
	else
	{
		if (pan_f < 0.0f) { pan_f = 0.0; }
		else if (pan_f > 1.0f) { pan_f = 1.0f; }

		// normalized -> unitless
		pan_use = convertNormalizedToUnitless(pan_f,
			unitless_limits->max_pan_value, unitless_limits->min_pan_value);
	}

	if (!tiltEnabled || !is_capability_supported("AX_PTZ_MOVE_ABS_TILT"))
	{
		tilt_use = AX_PTZ_MOVEMENT_NO_VALUE;
	}
	else
	{
		if (tilt_f < 0.0f) { tilt_f = 0.0; }
		else if (tilt_f > 1.0f) { tilt_f = 1.0f; }
		// normalized -> unitless
		tilt_use = convertNormalizedToUnitless(tilt_f,
			unitless_limits->max_tilt_value, unitless_limits->min_tilt_value);
	}

	if (!zoomEnabled || !is_capability_supported("AX_PTZ_MOVE_ABS_ZOOM"))
	{
		zoom_use = unitless_status->zoom_value;
	}
	else
	{
		if (zoom_f < 0.0f) { zoom_f = 0.0; }
		else if (zoom_f > 1.0f) { zoom_f = 1.0f; }
		// normalized -> unitless
		zoom_use = convertNormalizedToUnitless(zoom_f,
			unitless_limits->max_zoom_value, unitless_limits->min_zoom_value);
	}

	LOGINFO("(pan, tilt, zoom) = (%f, %f, %f)", pan_f, tilt_f, zoom_f);
	LOGINFO("speed = %f", speed);

	/*
	// only zoom if pan distance is small
	if (unitless_status->pan_value - pan_use > -10
	&& unitless_status->pan_value - pan_use < 10
	&& (unitless_status->zoom_value - zoom_use < -10
	|| unitless_status->zoom_value - zoom_use > 10))
	{
	pan_use = AX_PTZ_MOVEMENT_NO_VALUE;
	}*/


	// move
	if (!move_to_absolute_position(
		pan_use,
		tilt_use,
		AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		speed,
		AX_PTZ_MOVEMENT_PAN_TILT_SPEED_UNITLESS,
		zoom_use,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS))
	{
		goto failure;
	}

	// wait
	{
		gboolean is_moving = TRUE;
		/* Check if camera is moving */
		if (!(ax_ptz_movement_handler_is_ptz_moving
		(video_channel, &is_moving, &local_error)))
		{
			goto failure;
		}
		sleep(1);
	}


#ifdef AREA_CHECK
	{
		int i = 0;
		int j = 0;

		for (i = 0; i <= 10; i++)
		{
			for (j = 0; j <= 10; j++)
			{
				static AXPTZStatus *st = NULL;
				GError *er = NULL;
				float p_d = j / 10.0;
				float t_d = i / 10.0;
				float p_r;
				float t_r;

				move_to_absolute_position(
					convertNormalizedToUnitless(p_d,
						unitless_limits->max_pan_value, unitless_limits->min_pan_value),
					convertNormalizedToUnitless((t_d),// + 1.0f)*0.5f,
						unitless_limits->max_tilt_value, unitless_limits->min_tilt_value),
					AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
					0.7f,
					AX_PTZ_MOVEMENT_PAN_TILT_SPEED_UNITLESS,
					unitless_status->zoom_value,
					AX_PTZ_MOVEMENT_ZOOM_UNITLESS);
				sleep(8);
				ax_ptz_movement_handler_get_ptz_status(video_channel,
					AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
					AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
					&st,
					&er);

				p_r = convertUnitlessToNormalized(st->pan_value,
					unitless_limits->max_pan_value, unitless_limits->min_pan_value);


				t_r = convertUnitlessToNormalized(st->tilt_value,
					unitless_limits->max_tilt_value, unitless_limits->min_tilt_value);

				/*
				t_r = t_r * 2 - 1;

				if (t_r < 0)
				{
				t_r *= -1;

				if (p_r >= 0.5f)
				{
				p_r -= 0.5f;
				}
				}*/


				LOGINFO("%f, %f, %f, %f", p_d, t_d, p_r, t_r);

				sleep(1);

				g_free(st);
				st = NULL;
				if (er)
				{
					g_error_free(er);
					er = NULL;
				}
			}
		}
	}
#endif


	/* Now we don't need the axptz library anymore, destroy it */
	if (!(ax_ptz_destroy(&local_error)))
	{
		goto failure;
	}

	LOGINFO("%s finished successfully...\n", APP_NAME);

	/* Perform cleanup */

	if (local_error)
	{
		g_error_free(local_error);
		local_error = NULL;
	}

	if (capabilities)
	{
		for (it = g_list_first(capabilities); it != NULL; it = g_list_next(it))
		{
			g_free((gchar *)it->data);
		}
	}

	g_list_free(capabilities);
	capabilities = NULL;
	g_free(unitless_status);
	unitless_status = NULL;
	g_free(unitless_limits);
	unitless_limits = NULL;
	g_free(unit_limits);
	unit_limits = NULL;

	exit(EXIT_SUCCESS);

	/* We will end up here if something went wrong */
failure:

	if (local_error && local_error->message)
	{
		LOGINFO("ERROR: %s ended with errors:\n", APP_NAME);
		LOGINFO("%s\n", local_error->message);
	}

	if (local_error)
	{
		g_error_free(local_error);
		local_error = NULL;
	}

	/* Now we don't need the axptz library anymore, destroy it */
	ax_ptz_destroy(&local_error);

	/* Perform cleanup */

	if (local_error)
	{
		g_error_free(local_error);
		local_error = NULL;
	}

	if (capabilities)
	{
		for (it = g_list_first(capabilities); it != NULL; it = g_list_next(it))
		{
			g_free((gchar *)it->data);
		}
	}

	g_list_free(capabilities);
	capabilities = NULL;
	g_free(unitless_status);
	unitless_status = NULL;
	g_free(unitless_limits);
	unitless_limits = NULL;
	g_free(unit_limits);
	unit_limits = NULL;

#ifdef WRITE_TO_SYS_LOG
	closelog();
#endif

	exit(EXIT_FAILURE);
}
