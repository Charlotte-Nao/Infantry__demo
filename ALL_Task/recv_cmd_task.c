//
// Created by 14717 on 2026/3/19.
//

#include "recv_cmd_task.h"

#include "cmsis_os.h"
#include <stdlib.h>
#include <string.h>
#include "../Application/robot_global.h"
#include "../Application/auto_ctrl.h"
#include "../Bsp/usb_cdc/bsp_usb_cdc.h"
#include "../Bsp/uart/bsp_uart.h"

#define TEMP_UART1_RX_DEBUG 0
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

// 临时串口助手调试格式：valid,shoot,yaw,pitch[,vx,vy]\r\n
static int parse_target_data_from_uart(struct uart_device *uart, target_info_t *target)
{
	char buffer[100] = {0};
	int received_len = uart->Recv(uart, buffer, sizeof(buffer) - 1, 1);
	if (received_len <= 0) {
		return 0;
	}

	buffer[received_len] = '\0';

	if (strncmp(buffer, "PING", 4) == 0) {
		uart->Print(uart, "PONG\\r\\n");
		return 0;
	}

	char *token;
	char *rest = buffer;
	const char *delim = ",\r\n";

	token = strtok_r(rest, delim, &rest);
	if (!token) return 0;
	target->valid = (uint8_t)atoi(token);

	token = strtok_r(NULL, delim, &rest);
	if (!token) return 0;
	target->shoot = (uint8_t)atoi(token);

	token = strtok_r(NULL, delim, &rest);
	if (!token) return 0;
	target->aim_target_yaw = strtof(token, NULL);

	token = strtok_r(NULL, delim, &rest);
	if (!token) return 0;
	target->aim_target_pitch = strtof(token, NULL);

	token = strtok_r(NULL, delim, &rest);
	if (token) {
		target->chassis_vx = strtof(token, NULL);
		token = strtok_r(NULL, delim, &rest);
		if (token) {
			target->chassis_vy = strtof(token, NULL);
			target->chassis_vel_valid = 1U;
		} else {
			target->chassis_vx = 0.0f;
			target->chassis_vy = 0.0f;
			target->chassis_vel_valid = 0U;
		}
	} else {
		target->chassis_vx = 0.0f;
		target->chassis_vy = 0.0f;
		target->chassis_vel_valid = 0U;
	}

	return 1;
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

	struct uart_device* Uart = uart_get_device("uart1_dma");
	Uart->Init(Uart, 115200, 8, 'N', 1);
	last_chassis_cmd_tick = osKernelSysTick();

	for (;;)
	{
		uint8_t got_frame = 0U;

		if (usb == NULL)
		{
			usb = usb_get_device();
			if (usb != NULL)
			{
				usb->Init(usb);
				auto_aim_init(usb);
			}
		}

		if (usb != NULL && parse_target_data(&robot_ctrl.target_info) == 1)
		{
			got_frame = 1U;
			apply_target_result();
		}

#if TEMP_UART1_RX_DEBUG
		else if (parse_target_data_from_uart(Uart, &robot_ctrl.target_info) == 1)
		{
			got_frame = 1U;
			apply_target_result();
			Uart->Print(Uart, "UART1 frame ok: v=%d s=%d y=%.3f p=%.3f\\r\\n",
						robot_ctrl.target_info.valid,
						robot_ctrl.target_info.shoot,
						robot_ctrl.target_info.aim_target_yaw,
						robot_ctrl.target_info.aim_target_pitch);
		}
#endif

		if (!got_frame)
		{
			reset_target_to_hold();
		}
		process_chassis_cmd_timeout();

		osDelay(2);
	}
}

