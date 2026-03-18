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

void info_task_func(void const * argument) {

    struct uart_device* Uart = uart_get_device("uart1_dma");
    Uart->Init(Uart, 115200, 8, 'N', 1);

    struct usb_device* usb = usb_get_device();
    usb->Init(usb);

    while (1) {
        uint32_t current_tick = osKernelSysTick();  // 获取当前系统滴答定时器值(ms)，用于所有计时逻辑

        uint8_t robot_hp = global_info.super_cap->capacity_voltage;;

        usb->Print(usb, "HP: %d\n", robot_hp);  // 通过 USB CDC 输出当前 HP 信息，供上位机显示

        osDelay(2);  // 云台任务调度周期 2ms，固定频率保证控制精度
    }
}

