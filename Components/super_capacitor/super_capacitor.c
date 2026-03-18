#include "super_capacitor.h"

#include <string.h>

#include "../../Bsp/LED/bsp_LED.h"

static super_capacitor_data_t g_supercap_data;

static int16_t SuperCap_DecodeS16(const uint8_t high_or_low, const uint8_t low_or_high)
{
#if SUPERCAP_DATA_BIG_ENDIAN
	return (int16_t)(((uint16_t)high_or_low << 8) | (uint16_t)low_or_high);
#else
	return (int16_t)(((uint16_t)low_or_high << 8) | (uint16_t)high_or_low);
#endif
}

void SuperCap_Init(void)
{
	memset(&g_supercap_data, 0, sizeof(g_supercap_data));
}

uint8_t SuperCap_TryParse(const CAN_RxHeaderTypeDef *rx_header, const uint8_t rx_data[8])
{
	if (rx_header == NULL || rx_data == NULL) return 0U;

	if (rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA) return 0U;

	if (rx_header->DLC < 8U || rx_header->StdId != SUPERCAP_TX_STDID) return 0U;



	g_supercap_data.capacity_voltage = SuperCap_DecodeS16(rx_data[0], rx_data[1]);
	g_supercap_data.chassis_output_power = SuperCap_DecodeS16(rx_data[2], rx_data[3]);
	g_supercap_data.cap_charge_power = SuperCap_DecodeS16(rx_data[4], rx_data[5]);
	g_supercap_data.temperature = rx_data[6];
	g_supercap_data.status = rx_data[7];
	return 1U;
}

const super_capacitor_data_t *SuperCap_GetData(void)
{
	return &g_supercap_data;
}

uint8_t SuperCap_ReadPack(MY_CAN_Tx_Pack *pack)
{
	if (pack == NULL) return 0U;
	*pack = g_supercap_data;
	return 1U;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

	CAN_RxHeaderTypeDef rx_header;
	uint8_t rx_data[8];

	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK) return;

	(void)SuperCap_TryParse(&rx_header, rx_data);
}
