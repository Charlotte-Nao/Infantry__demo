//
// Created by 14717 on 2026/3/19.
//

#ifndef INFANTRY_01_SEND_INFO_H
#define INFANTRY_01_SEND_INFO_H

#include <stdint.h>

/* CAN2 outgoing IDs */
#define SEND_INFO_CAN2_MATCH_ID            0x301U
#define SEND_INFO_CAN2_TELEMETRY_ID        0x302U

/* RFID bytes in SEND_INFO_CAN2_TELEMETRY_ID */
#define SEND_INFO_ARMOR_ID_BYTE_INDEX       2U
#define SEND_INFO_CENTER_STATE_BYTE_INDEX   3U
#define SEND_INFO_RFID19_BYTE_INDEX         4U
#define SEND_INFO_RFID23_BYTE_INDEX         5U

void SendInfo_Init(void);
void SendInfo_CAN2_Periodic(void);
void SendInfo_GetTxDiag(uint32_t *ok_cnt, uint32_t *fail_cnt);

#endif //INFANTRY_01_SEND_INFO_H