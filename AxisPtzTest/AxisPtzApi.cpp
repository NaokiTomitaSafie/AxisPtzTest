/*
* Copyright (c) 2017 Safie Inc. All rights reserved.
*
* NOTICE: No part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Safie Inc.
*/

#define __STDC_LIMIT_MACROS

#include "AxisPtzApi.h"
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <stdio.h>

// The number of fractional bits used in fix-point variables
#define FIXMATH_FRAC_BITS 16

//#define MOVE_BY_ANOTHER_APP

// movement app path
#define PTZ_SETTER_APP_PATH "/safiecam/lib/ptzexecute/safiecam"

#define LOGINFO(fmt, args...) \
  do { \
    printf(fmt, ## args); \
    printf("\n"); \
  } while(0)

using namespace AxisPtzTest;


AxisPtzApi::AxisPtzApi() :
	video_channel(1),
	isAbsTiltSupported(false), isAbsPanSupported(false), isAbsZoomSupported(false),
	hasWholeLowerHemisphere(false), hasMotor(false),
	unitless_limits(NULL), unit_limits(NULL), 
	ax_ptz_control_queue_group(NULL),
	pan_value(fx_itox(0, FIXMATH_FRAC_BITS)),
	tilt_value(fx_itox(0, FIXMATH_FRAC_BITS)), 
	zoom_value(fx_itox(0, FIXMATH_FRAC_BITS)),
	moveSpeed(0.8f)
{


	this->isEnabled = false;
	if (!this->initialize())
	{
		LOGINFO("PTZ API is disabled");
		return;
	}

#ifdef MOVE_BY_ANOTHER_APP
	// Path of the executer app

	char selfPath[1024] = "";
	readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1);

	int separatorIndexPrev = -1;
	int separatorIndex = -1;

	for (int i = 0; i < (sizeof(selfPath) / sizeof(selfPath[0])); i++)
	{
		if (selfPath[i] == '\0')
		{
			break;
		}
		if (selfPath[i] == '/')
		{
			separatorIndexPrev = separatorIndex;
			separatorIndex = i;
		}
	}

	if (separatorIndexPrev < 0)
	{
		this->isEnabled = false;
		return;
	}

	std::string selfPathStr(selfPath);

	sprintf(this->executeAppPath, "%s%s",
		selfPathStr.substr(0, separatorIndexPrev).c_str(),
		PTZ_SETTER_APP_PATH);

	LOGINFO("%s", this->executeAppPath);
#endif

}


AxisPtzApi::~AxisPtzApi()
{
	GError* local_error = NULL;

	// destoy PTZ library
	if (!ax_ptz_destroy(&local_error))
	{
		g_error_free(local_error);
		local_error = NULL;
	}

	g_free(unitless_limits);
	unitless_limits = NULL;
	g_free(unit_limits);
	unit_limits = NULL;
}


bool AxisPtzApi::GetIsEnabled() const { return this->isEnabled; }

float AxisPtzApi::GetMoveSpeed() const { return this->moveSpeed; }
void AxisPtzApi::SetMoveSpeed(float value) { this->moveSpeed = value; }

bool AxisPtzApi::GetHasMotor() const { return this->hasMotor; }
void AxisPtzApi::SetHasMotor(bool value) { this->hasMotor = value; }


/**
	Check PTZ capability
*/
bool AxisPtzApi::get_ptz_move_capabilities()
{
	GError* local_error = NULL;
	GList* capabilities = NULL;

	this->isAbsTiltSupported = false;
	this->isAbsPanSupported = false;
	this->isAbsZoomSupported = false;


	if (!(capabilities = ax_ptz_movement_handler_get_move_capabilities
		(this->video_channel, &local_error)))
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}

	GList* it = NULL;

	for (it = g_list_first(capabilities); it != NULL; it = g_list_next(it))
	{
		if (!(g_strcmp0((gchar*)it->data, "AX_PTZ_MOVE_ABS_TILT")))
		{
			this->isAbsTiltSupported = true;
		}
		else if (!(g_strcmp0((gchar*)it->data, "AX_PTZ_MOVE_ABS_PAN")))
		{
			this->isAbsPanSupported = true;
		}
		else if (!(g_strcmp0((gchar*)it->data, "AX_PTZ_MOVE_ABS_ZOOM")))
		{
			this->isAbsZoomSupported = true;
		}

		g_free((gchar*)it->data);

	}

	g_list_free(capabilities);
	capabilities = NULL;

	return true;
}

/**
	Get limit
*/
bool AxisPtzApi::getLimits()
{
	GError* local_error = NULL;

	// Get the current status (e.g. the current pan/tilt/zoom value/position)
	if (!(ax_ptz_movement_handler_get_ptz_limits(
		this->video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&(this->unitless_limits),
		&local_error)))
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}


	LOGINFO("Unitless Maxpan:%f,Minpan:%f",
		fx_xtof(unitless_limits->max_pan_value, FIXMATH_FRAC_BITS),
		fx_xtof(unitless_limits->min_pan_value, FIXMATH_FRAC_BITS));

	LOGINFO("Unitless Maxtilt:%f,Mintilt:%f",
		fx_xtof(unitless_limits->max_tilt_value, FIXMATH_FRAC_BITS),
		fx_xtof(unitless_limits->min_tilt_value, FIXMATH_FRAC_BITS));

	LOGINFO("Unitless Maxzoom:%f,Minzoom:%f",
		fx_xtof(unitless_limits->max_zoom_value, FIXMATH_FRAC_BITS),
		fx_xtof(unitless_limits->min_zoom_value, FIXMATH_FRAC_BITS));


	// Get the pan, tilt and zoom limits for the unit (degrees) space
	if (!(ax_ptz_movement_handler_get_ptz_limits(
		this->video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_DEGREE,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&(this->unit_limits), &local_error)))
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}

	LOGINFO("Unit[deg] Maxpan:%f,Minpan:%f",
		fx_xtof(unit_limits->max_pan_value, FIXMATH_FRAC_BITS),
		fx_xtof(unit_limits->min_pan_value, FIXMATH_FRAC_BITS));

	LOGINFO("Unit[deg] Maxtilt:%f,Mintilt:%f",
		fx_xtof(unit_limits->max_tilt_value, FIXMATH_FRAC_BITS),
		fx_xtof(unit_limits->min_tilt_value, FIXMATH_FRAC_BITS));

	LOGINFO("Unit[deg] Maxzoom:%f,Minzoom:%f",
		fx_xtof(unit_limits->max_zoom_value, FIXMATH_FRAC_BITS),
		fx_xtof(unit_limits->min_zoom_value, FIXMATH_FRAC_BITS));

	float tilt_max = fx_xtof(unit_limits->max_tilt_value, FIXMATH_FRAC_BITS);
	float tilt_min = fx_xtof(unit_limits->min_tilt_value, FIXMATH_FRAC_BITS);

	if (tilt_max<45 && tilt_max>-45 && tilt_min<-135 && tilt_min>-225)
	{
		this->hasWholeLowerHemisphere = true;
		LOGINFO("hasWholeLowerHemisphere");
	}
	else
	{
		LOGINFO("not hasWholeLowerHemisphere");
	}

	return true;
}

/**
	Initialize PTZ API
*/
bool AxisPtzApi::initialize()
{
	GError* local_error = NULL;

	// Create the axptz library
	if (!(ax_ptz_create(&local_error)))
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}

	// Get limits
	if (!(getLimits()))
	{
		return false;
	}

	// Get the supported capabilities
	if (!(get_ptz_move_capabilities()))
	{
		return false;
	}

#ifndef MOVE_BY_ANOTHER_APP
	
	ax_ptz_control_queue_group = NULL;
	
	if (!(ax_ptz_control_queue_group =
		ax_ptz_control_queue_get_app_group_instance(&local_error)))
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}
#endif


	this->isEnabled = true;
	return true;
}


fixed_t AxisPtzApi::convertNormalizedToUnitless(float source, fixed_t max, fixed_t min)
{
	fixed_t gain = fx_ftox(source, FIXMATH_FRAC_BITS);
	return fx_addx(
		fx_mulx(max, gain, FIXMATH_FRAC_BITS),
		fx_mulx(min, fx_subx(fx_itox(1, FIXMATH_FRAC_BITS), gain), FIXMATH_FRAC_BITS));
}

float AxisPtzApi::convertUnitlessToNormalized(fixed_t source, fixed_t max, fixed_t min)
{
	fixed_t gain = fx_divx(fx_subx(source, min), fx_subx(max, min), FIXMATH_FRAC_BITS);
	return fx_xtof(gain, FIXMATH_FRAC_BITS);
}

/**
	Get current PTZ position in normalized space
*/
bool AxisPtzApi::GetPtzStatus(float& pan, float& tilt, float& zoom)
{
	if (!this->isEnabled) { return false; }

	fixed_t pan_ul, tilt_ul, zoom_ul;

	bool succeeded = this->getPtzStatusInner(pan_ul, tilt_ul, zoom_ul);
	
	// unitless -> normalized
	float p_r = this->convertUnitlessToNormalized(pan_ul,
		unitless_limits->max_pan_value, unitless_limits->min_pan_value);
	float t_r = this->convertUnitlessToNormalized(tilt_ul,
		unitless_limits->max_tilt_value, unitless_limits->min_tilt_value);
	zoom = this->convertUnitlessToNormalized(zoom_ul,
		unitless_limits->max_zoom_value, unitless_limits->min_zoom_value);


	// Coordinate transformation
	if (this->hasMotor && this->hasWholeLowerHemisphere)
	{
		t_r = t_r * 2.0f - 1.0f;

		if (t_r < 0)
		{
			t_r *= -1;

			if (p_r >= 0.5f)
			{
				p_r -= 0.5f;
			}
		}
	}

	pan = p_r;
	tilt = t_r;

	LOGINFO("(pan, tilt, zoom) = (%f, %f, %f)", pan, tilt, zoom);

	return succeeded;
}

/**
	Get current PTZ position in unitless space
*/
bool AxisPtzApi::getPtzStatusInner(fixed_t& pan, fixed_t& tilt, fixed_t& zoom)
{
	if (!this->isEnabled) { return false; }

	GError* local_error = NULL;
	AXPTZStatus* unitless_status = NULL;

	if (!(ax_ptz_movement_handler_get_ptz_status(this->video_channel,
		AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		AX_PTZ_MOVEMENT_ZOOM_UNITLESS,
		&unitless_status,
		&local_error)))
	{
		g_error_free(local_error);
		LOGINFO("get_status_failed.");
		return false;
	}

	pan = this->pan_value = unitless_status->pan_value;
	tilt = this->tilt_value = unitless_status->tilt_value;
	zoom = this->zoom_value = unitless_status->zoom_value;


	g_free(unitless_status);
	unitless_status = NULL;

	return true;
}

/**
	Request moving with parameters in normalized space
*/
bool AxisPtzApi::RequestPtzAbsolutePosition
(float pan, float tilt, float zoom, bool panEnabled, bool tiltEnabled, bool zoomEnabled)
{
	if (!this->isEnabled) { return false; }

	// Coordinate transformation
	if (this->hasMotor && this->hasWholeLowerHemisphere)
	{
		tilt = 0.5f*(tilt + 1.0f);
	}

#if 0
	fixed_t pan_ul, tilt_ul, zoom_ul;
	float _p, _t, _z;

	bool succeeded = this->getPtzStatusInner(pan_ul, tilt_ul, zoom_ul);

	// unitless space
	fixed_t pan_use, tilt_use, zoom_use;

	// normalized -> unitless
	pan_use = this->convertNormalizedToUnitless(pan,
		unitless_limits->max_pan_value, unitless_limits->min_pan_value);

	tilt_use = this->convertNormalizedToUnitless(tilt,
		unitless_limits->max_tilt_value, unitless_limits->min_tilt_value);

	zoom_use = this->convertNormalizedToUnitless(zoom,
		unitless_limits->max_zoom_value, unitless_limits->min_zoom_value);


	LOGINFO("diff PTZ : (%d, %d, %d)", pan_use - pan_ul, tilt_use - tilt_ul, zoom_use - zoom_ul);
#endif

#ifdef MOVE_BY_ANOTHER_APP
	char pstr[256] = "";


	if (panEnabled)
	{
		sprintf(pstr, "%s -p %10.6f", pstr, pan);
	}
	if (tiltEnabled)
	{
		sprintf(pstr, "%s -t %10.6f", pstr, tilt);
	}
	if (zoomEnabled)
	{
		sprintf(pstr, "%s -z %10.6f", pstr, zoom);
	}

	sprintf(pstr, "%s -s %10.6f", pstr, this->moveSpeed);

	char str[1024] = "";
	sprintf(str, "%s%s &", this->executeAppPath, pstr);

	int res = system(str);

#if 0
	LOGINFO("%s", str);
	//this->GetPtzStatus(_p, _t, _z);
#endif

	return res >= 0;
#else

	// unitless space
	fixed_t pan_use, tilt_use, zoom_use;

	if (!panEnabled || !this->isAbsPanSupported)
	{
		pan_use = AX_PTZ_MOVEMENT_NO_VALUE;
	}
	else
	{
		if (pan < 0.0f) { pan = 0.0; }
		else if (pan > 1.0f) { pan = 1.0f; }

		// normalized -> unitless
		pan_use = this->convertNormalizedToUnitless(pan,
			unitless_limits->max_pan_value, unitless_limits->min_pan_value);
	}

	if (!tiltEnabled || !this->isAbsTiltSupported)
	{
		tilt_use = AX_PTZ_MOVEMENT_NO_VALUE;
	}
	else
	{
		if (tilt < 0.0f) { tilt = 0.0; }
		else if (tilt > 1.0f) { tilt = 1.0f; }
		// normalized -> unitless
		tilt_use = this->convertNormalizedToUnitless(tilt,
			unitless_limits->max_tilt_value, unitless_limits->min_tilt_value);
	}

	if (!zoomEnabled || !this->isAbsZoomSupported)
	{
		// unitless space
		fixed_t p, t, currentZoom;
		this->getPtzStatusInner(p, t, currentZoom);
		zoom_use = currentZoom;
	}
	else
	{
		if (zoom < 0.0f) { zoom = 0.0; }
		else if (zoom > 1.0f) { zoom = 1.0f; }
		// normalized -> unitless
		zoom_use = this->convertNormalizedToUnitless(zoom,
			unitless_limits->max_zoom_value, unitless_limits->min_zoom_value);
	}

	LOGINFO("(pan, tilt, zoom) = (%f, %f, %f)", pan, tilt, zoom);

	if (!(move_to_absolute_position(
		//library,
		pan_use, tilt_use, AX_PTZ_MOVEMENT_PAN_TILT_UNITLESS,
		this->moveSpeed, AX_PTZ_MOVEMENT_PAN_TILT_SPEED_UNITLESS,
		zoom_use, AX_PTZ_MOVEMENT_ZOOM_UNITLESS)))
	{
		return false;
	}

	float _p, _t, _z;
	this->GetPtzStatus(_p, _t, _z);

	return true;

#endif
}

/**
Perform camera movement to absolute position
*/
bool AxisPtzApi::move_to_absolute_position(
	fixed_t pan_p,
	fixed_t tilt_p,
	AXPTZMovementPanTiltSpace pan_tilt_space,
	float speed,
	AXPTZMovementPanTiltSpeedSpace pan_tilt_speed_space,
	fixed_t zoom_p, AXPTZMovementZoomSpace zoom_space)
{

	AXPTZAbsoluteMovement* abs_movement = NULL;
	GError* local_error = NULL;


	// Set the unit spaces for an absolute movement
	if ((ax_ptz_movement_handler_set_absolute_spaces
	(pan_tilt_space, pan_tilt_speed_space, zoom_space, &local_error)))
	{
		// Create an absolute movement structure
		if ((abs_movement = ax_ptz_absolute_movement_create(&local_error)))
		{

			// Set the pan, tilt and zoom values for the absolute movement
			if (!(ax_ptz_absolute_movement_set_pan_tilt_zoom(abs_movement,
				pan_p,
				tilt_p,
				fx_ftox(speed, FIXMATH_FRAC_BITS),
				zoom_p,
				AX_PTZ_MOVEMENT_NO_VALUE,
				&local_error)))
			{
				ax_ptz_absolute_movement_destroy(abs_movement, NULL);
				g_error_free(local_error);
				local_error = NULL;
				return false;
			}

			// Perform the absolute movement
			if (!(ax_ptz_movement_handler_absolute_move(ax_ptz_control_queue_group,
				video_channel,
				abs_movement,
				AX_PTZ_INVOKE_ASYNC, NULL,
				NULL, &local_error)))
			{
				ax_ptz_absolute_movement_destroy(abs_movement, NULL);
				g_error_free(local_error);
				local_error = NULL;
				return false;
			}


			// Now we don't need the absolute movement structure anymore, destroy it
			if (!(ax_ptz_absolute_movement_destroy(abs_movement, &local_error)))
			{
				g_error_free(local_error);
				local_error = NULL;
				return false;
			}
		}
		else
		{
			g_error_free(local_error);
			local_error = NULL;
			return false;
		}
	}
	else
	{
		g_error_free(local_error);
		local_error = NULL;
		return false;
	}

	return true;
}
