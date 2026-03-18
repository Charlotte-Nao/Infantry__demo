#include "../ALL_Task/ui_task.h"
#include "../Bsp/uart/bsp_uart.h"
#include "../Application/global_info.h"
#include "cmsis_os.h"
#include "../Bsp/LED/bsp_LED.h"


void ui_task_func(void const * argument) {

    struct uart_device* Uart = uart_get_device("uart1_dma");
    Uart->Init(Uart, 115200, 8, 'N', 1);

    while (1) {
        uint32_t current_tick = osKernelSysTick();

        osDelay(2);
    }
}

