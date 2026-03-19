//
// Created by 14717 on 2026/3/19.
//

#include "recv_cmd_task.h"

#include "cmsis_os.h"
#include "../Application/robot_global.h"
#include "../Application/auto_aim.h"
#include "../Bsp/usb_cdc/bsp_usb_cdc.h"

void recv_cmd_task_func(void const * argument)
{
	struct usb_device *usb = NULL;
	(void)argument;

	for (;;)
	{
		if (usb == NULL)
		{
			usb = usb_get_device();
			if (usb != NULL)
			{
				usb->Init(usb);
				auto_aim_init(usb);
			}
			else
			{
				robot_ctrl.target_info.valid = 0;
				robot_ctrl.target_info.shoot = 0;
				robot_ctrl.target_info.aim_target_yaw = robot_ctrl.gimbal.yaw;
				robot_ctrl.target_info.aim_target_pitch = robot_ctrl.gimbal.pitch;
				robot_ctrl.monitor.vision_online = 0;
				osDelay(10);
				continue;
			}
		}

		if (parse_target_data(&robot_ctrl.target_info) == 1)
		{
			robot_ctrl.monitor.vision_online = 1;
		}
		else
		{
			robot_ctrl.target_info.valid = 0;
			robot_ctrl.target_info.shoot = 0;
			robot_ctrl.target_info.aim_target_yaw = robot_ctrl.gimbal.yaw;
			robot_ctrl.target_info.aim_target_pitch = robot_ctrl.gimbal.pitch;
			robot_ctrl.monitor.vision_online = 0;
		}

		osDelay(2);
	}
}

