#include "send_info_task.h"

#include "cmsis_os.h"

#include "../Application/robot_global.h"
#include "../Bsp/usb_cdc/bsp_usb_cdc.h"

void send_info_task_func(void const * argument)
{
	struct usb_device *usb = usb_get_device();
	(void)argument;

	if (usb != NULL)
	{
		usb->Init(usb);
	}

	for (;;) {
		if (usb != NULL) {
			usb->Print(usb,
			           "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\r\n",
			           robot_ctrl.gimbal.q[0],
			           robot_ctrl.gimbal.q[1],
			           robot_ctrl.gimbal.q[2],
			           robot_ctrl.gimbal.q[3],
			           robot_ctrl.gimbal.yaw,
			           robot_ctrl.gimbal.pitch,
			           // robot_ctrl.game_info.stage_remain_time,
			           // robot_ctrl.game_info.current_HP,
			           robot_ctrl.game_info.robot_id);
		}

		osDelay(1);
	}
}
