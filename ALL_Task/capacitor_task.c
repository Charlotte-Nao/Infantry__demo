#include "capacitor_task.h"
#include "math.h"
#include "cmsis_os.h"
#include "../Bsp/usb_cdc/bsp_usb_cdc.h"

void capacitor_task_func(void const * argument) {
    while (1) {
        vTaskDelay(1);
    }
}