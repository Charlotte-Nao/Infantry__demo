//
// Created by 14717 on 2026/3/19.
//

#include "recv_cmd_task.h"

#include "cmsis_os.h"
#include "../Application/robot_global.h"
#include "../Application/auto_ctrl.h"
#include "../Bsp/usb_cdc/bsp_usb_cdc.h"

#define CHASSIS_CMD_HOLD_TIMEOUT_MS 250U

static uint32_t last_chassis_cmd_tick = 0U;

static void reset_target_to_hold(void)
{
	robot_ctrl.target_info.valid = 0;
	robot_ctrl.target_info.shoot = 0;
	robot_ctrl.target_info.aim_target_yaw = robot_ctrl.gimbal.yaw;
	robot_ctrl.target_info.aim_target_pitch = robot_ctrl.gimbal.pitch;
	robot_ctrl.target_info.chassis_vx = 0.0f;
	robot_ctrl.target_info.chassis_vy = 0.0f;
	robot_ctrl.target_info.chassis_vel_valid = 0U;
	robot_ctrl.monitor.vision_online = 0;
}

static void process_chassis_cmd_timeout(void)
{
	uint32_t now = osKernelSysTick();
	if ((uint32_t)(now - last_chassis_cmd_tick) >= CHASSIS_CMD_HOLD_TIMEOUT_MS) {
		robot_ctrl.chassis.cmd_vx = 0.0f;
		robot_ctrl.chassis.cmd_vy = 0.0f;
	}
}

static void apply_target_result(void)
{
	robot_ctrl.monitor.vision_online = is_target_valid(&robot_ctrl.target_info) ? 1U : 0U;
	if (robot_ctrl.target_info.chassis_vel_valid) {
		robot_ctrl.chassis.cmd_vx = robot_ctrl.target_info.chassis_vx;
		robot_ctrl.chassis.cmd_vy = robot_ctrl.target_info.chassis_vy;
		last_chassis_cmd_tick = osKernelSysTick();
	}
}

void recv_cmd_task_func(void const * argument)
{
	struct usb_device *usb = NULL;
	(void)argument;
	last_chassis_cmd_tick = osKernelSysTick();

	for (;;)
	{
		uint8_t got_frame = 0U;
		int usb_ret = 0;

		if (usb == NULL)
		{
			usb = usb_get_device();
			if (usb != NULL)
			{
				usb->Init(usb);
				auto_aim_init(usb);
			}
		}

		if (usb != NULL)
		{
			usb_ret = parse_target_data(&robot_ctrl.target_info);
			if (usb_ret == 1)
			{
				got_frame = 1U;
				apply_target_result();
			}
		}

		if (!got_frame)
		{
			reset_target_to_hold();
		}
		process_chassis_cmd_timeout();

		osDelay(2);
	}
}

