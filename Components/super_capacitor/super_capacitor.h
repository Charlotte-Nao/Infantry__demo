//
// Created by 14717 on 2026/3/19.
//

#ifndef INFANTRY_01_SUPER_CAPACITOR_H
#define INFANTRY_01_SUPER_CAPACITOR_H

#include "stm32f4xx_hal.h"

typedef struct super_capacitor_data_t
{
	int16_t capacity_voltage;      /* x100 */
	int16_t chassis_output_power;  /* x10 */
	int16_t cap_charge_power;      /* x10 */
	uint8_t temperature;
	uint8_t status;
} __attribute__((packed)) super_capacitor_data_t;

		/* Compatible command set from MY_CAN protocol. */
		typedef enum
		{
			CAP_AUTO = 0x00,
			CAP_OUTPUT_DISABLE = 0x01,
			CAP_WIRELESS_CHARGING = 0x02,
			CAP_SWITCH_OFF = 0x03,
		} super_cap_cmd_t;

		/* Backward-compatible type alias for external MY_CAN naming. */
		typedef super_capacitor_data_t MY_CAN_Tx_Pack;

/* Super capacitor feedback StdId (modify to your actual BMS ID if needed). */
#ifndef SUPERCAP_RX_STDID
#define SUPERCAP_RX_STDID 0x300U
#endif

#ifndef SUPERCAP_TX_STDID
#define SUPERCAP_TX_STDID 0x301U
#endif

#ifndef SUPERCAP_TX_CAP_OUTPUT_STDID
#define SUPERCAP_TX_CAP_OUTPUT_STDID 0x302U
#endif

/* 1: data[0] is high byte, 0: data[0] is low byte. */
#ifndef SUPERCAP_DATA_BIG_ENDIAN
#define SUPERCAP_DATA_BIG_ENDIAN 0
#endif

void SuperCap_Init(void);
uint8_t SuperCap_TryParse(const CAN_RxHeaderTypeDef *rx_header, const uint8_t rx_data[8]);
const super_capacitor_data_t *SuperCap_GetData(void);
uint8_t SuperCap_ReadPack(MY_CAN_Tx_Pack *pack);

#endif //INFANTRY_01_SUPER_CAPACITOR_H