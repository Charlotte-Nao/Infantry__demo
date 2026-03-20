#include "send_info.h"

#include "can.h"
#include "cmsis_os.h"
#include "../global_info.h"
#include "../../Components/referee/referee.h"
#include "../../Components/super_capacitor/super_capacitor.h"

static uint32_t s_last_match_tick;
static uint32_t s_last_telemetry_tick;
static uint32_t s_tx_ok_cnt;
static uint32_t s_tx_fail_cnt;

static uint8_t SendInfo_CAN2_Send(uint32_t std_id, const uint8_t data[8])
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t mailbox;

    tx_header.StdId = std_id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8U;
    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) == 0U) {
        s_tx_fail_cnt++;
        return 0U;
    }

    if (HAL_CAN_AddTxMessage(&hcan2, &tx_header, (uint8_t *)data, &mailbox) == HAL_OK) {
        s_tx_ok_cnt++;
        return 1U;
    }

    s_tx_fail_cnt++;
    return 0U;
}

void SendInfo_Init(void)
{
    s_last_match_tick = 0U;
    s_last_telemetry_tick = 0U;
    s_tx_ok_cnt = 0U;
    s_tx_fail_cnt = 0U;
}

void SendInfo_GetTxDiag(uint32_t *ok_cnt, uint32_t *fail_cnt)
{
    if (ok_cnt != NULL) {
        *ok_cnt = s_tx_ok_cnt;
    }
    if (fail_cnt != NULL) {
        *fail_cnt = s_tx_fail_cnt;
    }
}

void SendInfo_CAN2_Periodic(void)
{
    uint32_t now = osKernelSysTick();

    if (global_info.referee != NULL && global_info.super_cap != NULL && (now - s_last_match_tick) >= 20U)
    {
        uint8_t data[8] = {0};
        const game_info_t *ref = global_info.referee;
        const super_capacitor_data_t *cap = global_info.super_cap;

        /* Frame 0x301:
         * [0] robot_id
         * [1] game_progress
         * [2..3] stage_remain_time (LE)
         * [4..5] current_HP (LE)
         * [6..7] capacity_voltage (LE, x100)
         */
        data[0] = ref->robot_status.robot_id;
        data[1] = ref->game_status.game_progress;
        data[2] = (uint8_t)(ref->game_status.stage_remain_time & 0xFFU);
        data[3] = (uint8_t)((ref->game_status.stage_remain_time >> 8) & 0xFFU);
        data[4] = (uint8_t)(ref->robot_status.current_HP & 0xFFU);
        data[5] = (uint8_t)((ref->robot_status.current_HP >> 8) & 0xFFU);
        data[6] = (uint8_t)(cap->capacity_voltage & 0xFF);
        data[7] = (uint8_t)((cap->capacity_voltage >> 8) & 0xFF);

        (void)SendInfo_CAN2_Send(SEND_INFO_CAN2_MATCH_ID, data);
        s_last_match_tick = now;
    }

    if (global_info.referee != NULL && (now - s_last_telemetry_tick) >= 20U)
    {
        uint8_t data[8] = {0};
        const game_info_t *ref = global_info.referee;
        uint8_t rfid19 = 0U;
        uint8_t rfid23 = 0U;
        uint8_t center_bonus_state = 0U;

        /* rfid bit19 and bit23 are in rfid_status (bit0..31). */
        rfid19 = (uint8_t)((ref->rfid_status.rfid_status >> 19) & 0x1U);
        rfid23 = (uint8_t)((ref->rfid_status.rfid_status >> 23) & 0x1U);
        /* event_data bit23-24: 0=none,1=ally,2=enemy,3=both */
        center_bonus_state = (uint8_t)((ref->event_data.event_data >> 23) & 0x03U);

        /* Frame 0x302:
         * [0..1] shooter_17mm_barrel_heat (LE)
         * [2] armor_id (hurt_data bit0-3)
         * [3] center_bonus_state (event_data bit23-24, 0~3)
         * [4] rfid bit19 (0/1)
         * [5] rfid bit23 (0/1)
         * [6..7] reserved
         */
        data[0] = (uint8_t)(ref->power_heat_data.shooter_17mm_barrel_heat & 0xFFU);
        data[1] = (uint8_t)((ref->power_heat_data.shooter_17mm_barrel_heat >> 8) & 0xFFU);
        data[2] = (uint8_t)(ref->hurt_data.armor_id & 0x0FU);
        data[3] = center_bonus_state;
        data[4] = rfid19;
        data[5] = rfid23;

        (void)SendInfo_CAN2_Send(SEND_INFO_CAN2_TELEMETRY_ID, data);
        s_last_telemetry_tick = now;
    }
}

