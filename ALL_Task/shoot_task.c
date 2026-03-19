#include "shoot_task.h"

#include "cmsis_os.h"
#include "math.h"

#include "../Application/robot_global.h"
#include "../Components/motor/motor.h"
#include "../Components/remote/remote.h"

/* 500Hz shoot control parameters */
#define SHOOT_FW_SPEED         6000.0f
#define STIR_REVERSE_SPEED     2500.0f
#define SHOOT_HEAT_LIMIT_17MM  200U

/* Trigger plate step config (output side). */
#define STIR_ENCODER_CPR           8192.0f
#define STIR_GEAR_RATIO            36.0f   /* 减速箱 36:1 */
#define STIR_BELT_RATIO            2.8f    /* 同步带比，可在线调整 */
#define STIR_TOTAL_RATIO           (STIR_GEAR_RATIO * STIR_BELT_RATIO)
#define STIR_STEP_OUTPUT_DEG       36.0f   /* 单击拨弹角度（输出侧�� */
#define STIR_STEP_DIR              (-1.0f) /* 方向不对改为 +1.0f */

#define STIR_STEP_TICKS ((int32_t)(STIR_STEP_DIR * STIR_ENCODER_CPR * STIR_TOTAL_RATIO * (STIR_STEP_OUTPUT_DEG / 360.0f)))

void shoot_task_func(void const * argument)
{
	struct motor_device *shoot_l = motor_get_device("M3508_SHOOT_L");
	struct motor_device *shoot_r = motor_get_device("M3508_SHOOT_R");
	struct motor_device *stir_m  = motor_get_device("M2006_TRIGGER");

	int32_t stir_pos_sum = 0;
	int32_t stir_target_sum = 0;
	uint8_t stir_target_inited = 0U;
	uint8_t last_fire_btn = 0U;
	(void)argument;

	for (;;)
	{
		uint8_t fire_cmd = 0U;
		uint8_t reverse_cmd;
		uint8_t heat_block = 0U;
		uint8_t fw_offline_block = 0U;
		uint8_t feed_block = 0U;

		if (shoot_l == NULL || shoot_r == NULL || stir_m == NULL)
		{
			osDelay(2);
			continue;
		}

		if (robot_ctrl.shoot_mode == SHOOT_READY)
		{
			shoot_l->set_target(shoot_l, 1, SHOOT_FW_SPEED);
			shoot_r->set_target(shoot_r, 1, -SHOOT_FW_SPEED);
		}
		else
		{
			shoot_l->set_target(shoot_l, 1, 0);
			shoot_r->set_target(shoot_r, 1, 0);
		}

		stir_m->get_status(stir_m, "POS_SUM", &stir_pos_sum);
		if (stir_target_inited == 0U)
		{
			stir_target_sum = stir_pos_sum;
			stir_target_inited = 1U;
		}

		/* C 档用于强制停转，不再作为拨弹反转触发。 */
		reverse_cmd = robot_ctrl.rc->vt13.mouse_vt13.press_m;
		heat_block = (robot_ctrl.game_info.shooter_17mm_barrel_heat > SHOOT_HEAT_LIMIT_17MM) ? 1U : 0U;
		fw_offline_block = (robot_ctrl.motors_info.m3508_shoot_l.online == 0U ||
							robot_ctrl.motors_info.m3508_shoot_r.online == 0U) ? 1U : 0U;
		feed_block = (heat_block != 0U || fw_offline_block != 0U) ? 1U : 0U;

		/* 过热或摩擦轮掉线时只禁止拨弹：摩擦轮照常转��拨弹目标锁定当前位置 */
		if (feed_block != 0U)
		{
			stir_target_sum = stir_pos_sum;
			last_fire_btn = 0U;
		}

		if (robot_ctrl.gimbal_mode == GIMBAL_REMOTE)
		{
			fire_cmd = (robot_ctrl.rc->vt13.mouse_vt13.press_l || robot_ctrl.rc->vt13.rc_vt13.trigger)
					 && (robot_ctrl.shoot_mode == SHOOT_READY)
					 && (feed_block == 0U);
		}
		else if (robot_ctrl.gimbal_mode == GIMBAL_AUTO)
		{
			fire_cmd = (robot_ctrl.rc->vt13.mouse_vt13.press_l || robot_ctrl.rc->vt13.rc_vt13.trigger)
					 && (robot_ctrl.shoot_mode == SHOOT_READY)
					 && (robot_ctrl.target_info.shoot == 1U)
					 && (feed_block == 0U);
		}

		if (reverse_cmd)
		{
			/* para_num=2: speed override mode */
			stir_m->set_target(stir_m, 2, STIR_REVERSE_SPEED, 1.0);
			stir_target_sum = stir_pos_sum;
			last_fire_btn = 0U;
		}
		else
		{
			if (fire_cmd && (last_fire_btn == 0U))
			{
				stir_target_sum += STIR_STEP_TICKS;
			}

			/* para_num=1: position mode target */
			stir_m->set_target(stir_m, 1, (double)stir_target_sum);
			last_fire_btn = fire_cmd;
		}

		osDelay(2);
	}
}
