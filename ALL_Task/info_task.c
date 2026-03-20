#include "info_task.h"          // 云台任务头文件-本文件声明
#include "../Application/global_info.h"  // 全局变量头文件-核心全局结构体、枚举定义
#include "../../Components/motor/motor.h" // 电机驱动头文件-电机设备句柄/接口函数
#include "math.h"                 // 数学库头文件-三角函数/绝对值/浮点运算
#include "stdlib.h"               // 标准库头文件-通用工具函数
#include "cmsis_os.h"             // RTOS系统头文件-系统滴答/延时/任务调度
#include "../../Bsp/uart/bsp_uart.h" // 串口驱动头文件-上位机/外设通信
#include "../../Bsp/usb_cdc/bsp_usb_cdc.h"
#include "../../Bsp/led/bsp_led.h"   // LED驱动头文件-状态指示灯控制【保留灯光 不删除】
#include "../Components/referee/referee.h"
#include "../Components/super_capacitor/super_capacitor.h"
#include "../Application/send_info/send_info.h"

#define HEAT_LOG_PERIOD_MS         50U
#define HEAT_STALE_TIMEOUT_MS      300U
#define CAN_TX_LOG_PERIOD_MS       200U
#define HEAT_LOG_ENABLE            0U

void info_task_func(void const * argument) {

    struct uart_device* Uart = uart_get_device("uart1_dma");
    Uart->Init(Uart, 115200, 8, 'N', 1);

    struct usb_device* usb = usb_get_device();
    usb->Init(usb);

    SendInfo_Init();

    uint16_t last_heat = 0xFFFFU;
    uint32_t last_log_tick = 0U;
    uint32_t last_heat_update_count = 0U;
    uint8_t heat_stale_reported = 0U;
    uint32_t last_can_log_tick = 0U;
    uint32_t last_tx_ok = 0U;
    uint32_t last_tx_fail = 0U;

    while (1) {
        uint32_t now = osKernelSysTick();
        SendInfo_CAN2_Periodic();

        if ((now - last_can_log_tick) >= CAN_TX_LOG_PERIOD_MS)
        {
            uint32_t tx_ok = 0U;
            uint32_t tx_fail = 0U;
            SendInfo_GetTxDiag(&tx_ok, &tx_fail);

            Uart->Print(Uart,
                        "[CAN2TX] t=%lu ok=%lu fail=%lu d_ok=%ld d_fail=%ld\r\n",
                        (unsigned long)now,
                        (unsigned long)tx_ok,
                        (unsigned long)tx_fail,
                        (long)(tx_ok - last_tx_ok),
                        (long)(tx_fail - last_tx_fail));

            last_tx_ok = tx_ok;
            last_tx_fail = tx_fail;
            last_can_log_tick = now;
        }


        if (global_info.referee != NULL)
        {
            uint16_t heat = global_info.referee->power_heat_data.shooter_17mm_barrel_heat;
            uint32_t heat_tick = global_info.referee->power_heat_last_update_tick;
            uint32_t heat_cnt = global_info.referee->power_heat_update_count;
            uint32_t heat_age = now - heat_tick;

            if ((heat != last_heat) || (heat_cnt != last_heat_update_count) || ((now - last_log_tick) >= HEAT_LOG_PERIOD_MS))
            {
                if (HEAT_LOG_ENABLE) {
                    Uart->Print(Uart,
                                "[HEAT] t=%lu v=%u age=%lu cnt=%lu\r\n",
                                (unsigned long)now,
                                (unsigned int)heat,
                                (unsigned long)heat_age,
                                (unsigned long)heat_cnt);
                }
                last_heat = heat;
                last_heat_update_count = heat_cnt;
                last_log_tick = now;
            }

            if ((heat_age > HEAT_STALE_TIMEOUT_MS) && (heat_stale_reported == 0U))
            {
                if (HEAT_LOG_ENABLE) {
                    Uart->Print(Uart,
                                "[HEAT][STALE] t=%lu age=%lu last=%u cnt=%lu\r\n",
                                (unsigned long)now,
                                (unsigned long)heat_age,
                                (unsigned int)heat,
                                (unsigned long)heat_cnt);
                }
                heat_stale_reported = 1U;
            }
            else if ((heat_age <= HEAT_STALE_TIMEOUT_MS) && (heat_stale_reported != 0U))
            {
                if (HEAT_LOG_ENABLE) {
                    Uart->Print(Uart,
                                "[HEAT][RECOVER] t=%lu age=%lu v=%u cnt=%lu\r\n",
                                (unsigned long)now,
                                (unsigned long)heat_age,
                                (unsigned int)heat,
                                (unsigned long)heat_cnt);
                }
                heat_stale_reported = 0U;
            }
        }

        //uint16_t temp = global_info.super_cap->temperature;
        //uint16_t cap_v = global_info.super_cap->capacity_voltage;

        //usb->Print(usb, "SuperCap: Temp=%dC, Volt=%dV\r\n", temp, cap_v);

        osDelay(2);  // 云台任务调度周期 2ms，固定频率保证控制精度
    }
}

