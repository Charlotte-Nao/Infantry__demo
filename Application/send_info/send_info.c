#include "send_info.h"

#include "can.h"
#include "cmsis_os.h"
#include "../global_info.h"
#include "../../Components/referee/referee.h"
#include "../../Components/super_capacitor/super_capacitor.h"

static uint32_t s_last_match_tick;
static uint32_t s_last_cap_tick;

static uint8_t SendInfo_CAN2_Send(uint32_t std_id, const uint8_t data[8])
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t mailbox;

    tx_header.StdId = std_id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8U;
    tx_header.TransmitGlobalTime = DISABLE;

    return (HAL_CAN_AddTxMessage(&hcan2, &tx_header, (uint8_t *)data, &mailbox) == HAL_OK) ? 1U : 0U;
}

static uint16_t SendInfo_CapacityPercentX10(int16_t cap_voltage_x100)
{
    int32_t min_v = SEND_INFO_CAP_VOLT_MIN_X100;
    int32_t max_v = SEND_INFO_CAP_VOLT_MAX_X100;
    int32_t range = max_v - min_v;
    int32_t val;

    if (range <= 0) return 0U;
    val = ((int32_t)cap_voltage_x100 - min_v) * 1000 / range;
    if (val < 0) val = 0;
    if (val > 1000) val = 1000;
    return (uint16_t)val;
}

void SendInfo_Init(void)
{
    s_last_match_tick = 0U;
    s_last_cap_tick = 0U;
}

void SendInfo_CAN2_Periodic(void)
{
    uint32_t now = osKernelSysTick();

    if (global_info.referee != NULL && (now - s_last_match_tick) >= 20U)
    {
        uint8_t data[8] = {0};
        const game_info_t *ref = global_info.referee;

        data[0] = ref->game_status.game_type;
        data[1] = ref->game_status.game_progress;
        data[2] = (uint8_t)(ref->game_status.stage_remain_time & 0xFFU);
        data[3] = (uint8_t)((ref->game_status.stage_remain_time >> 8) & 0xFFU);
        data[4] = (uint8_t)(ref->robot_status.current_HP & 0xFFU);
        data[5] = (uint8_t)((ref->robot_status.current_HP >> 8) & 0xFFU);
        data[6] = (uint8_t)(ref->robot_status.maximum_HP & 0xFFU);
        data[7] = (uint8_t)((ref->robot_status.maximum_HP >> 8) & 0xFFU);

        (void)SendInfo_CAN2_Send(SEND_INFO_CAN2_MATCH_ID, data);
        s_last_match_tick = now;
    }

    if (global_info.super_cap != NULL && (now - s_last_cap_tick) >= 20U)
    {
        uint8_t data[8] = {0};
        const super_capacitor_data_t *cap = global_info.super_cap;
        uint16_t percent_x10 = SendInfo_CapacityPercentX10(cap->capacity_voltage);

        data[0] = (uint8_t)(cap->capacity_voltage & 0xFF);
        data[1] = (uint8_t)((cap->capacity_voltage >> 8) & 0xFF);
        data[2] = (uint8_t)(cap->chassis_output_power & 0xFF);
        data[3] = (uint8_t)((cap->chassis_output_power >> 8) & 0xFF);
        data[4] = (uint8_t)(percent_x10 & 0xFFU);
        data[5] = (uint8_t)((percent_x10 >> 8) & 0xFFU);
        data[6] = cap->temperature;
        data[7] = cap->status;

        (void)SendInfo_CAN2_Send(SEND_INFO_CAN2_SUPERCAP_ID, data);
        s_last_cap_tick = now;
    }
}

