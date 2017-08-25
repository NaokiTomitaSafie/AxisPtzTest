/*
* Copyright (c) 2017 Safie Inc. All rights reserved.
*
* NOTICE: No part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Safie Inc.
*/

#ifndef __AxisPtzApi__
#define __AxisPtzApi__

#include <fixmath.h>
#include <axsdk/axptz.h>

namespace AxisPtzTest
{
	using namespace AxisPtzTest;

	/**
		for AXIS camera Pan/Tilt/Zoom API
	*/
	class AxisPtzApi
	{

	public:
		AxisPtzApi();
		~AxisPtzApi();

		bool GetIsEnabled() const;

		float GetMoveSpeed() const;
		void SetMoveSpeed(float value);

		bool GetHasMotor() const;
		void SetHasMotor(bool value);

		/**
		Get current PTZ position in normalized space
		*/
		bool GetPtzStatus(float& pan, float& tilt, float& zoom);

		/**
		Request moving with parameters in normalized space
		*/
		bool RequestPtzAbsolutePosition
		(float pan, float tilt, float zoom, bool panEnabled, bool tiltEnabled, bool zoomEnabled);

	private:
		
		bool isEnabled;

		gint video_channel;
		bool isAbsTiltSupported;
		bool isAbsPanSupported;
		bool isAbsZoomSupported;

		bool hasWholeLowerHemisphere;
		bool hasMotor;

		char executeAppPath[1024];

		AXPTZLimits *unitless_limits;
		AXPTZLimits *unit_limits;

		AXPTZControlQueueGroup* ax_ptz_control_queue_group;

		fixed_t pan_value, tilt_value, zoom_value;
		float moveSpeed;

		fixed_t convertNormalizedToUnitless(float source, fixed_t max, fixed_t min);
		float convertUnitlessToNormalized(fixed_t source, fixed_t max, fixed_t min);

		/**
		Check PTZ capability
		*/
		bool get_ptz_move_capabilities();

		/**
		Get limit
		*/
		bool getLimits();

		/**
		Initialize PTZ API
		*/
		bool initialize();

		/**
		Get current PTZ position in unitless space
		*/
		bool getPtzStatusInner(fixed_t& pan, fixed_t& tilt, fixed_t& zoom);

		/**
		Perform camera movement to absolute position
		*/
		bool move_to_absolute_position(
			fixed_t pan_value,
			fixed_t tilt_value,
			AXPTZMovementPanTiltSpace pan_tilt_space,
			float speed,
			AXPTZMovementPanTiltSpeedSpace pan_tilt_speed_space,
			fixed_t zoom_value, AXPTZMovementZoomSpace zoom_space);
	};
}

#endif