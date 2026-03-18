#include "global_info.h"
#include "string.h"
#include "../Components/referee/referee.h"
#include "../Components/super_capacitor/super_capacitor.h"

/* 实例化全局控制变量 */
global_info_t global_info;

/**
 * @brief 全局控制变量初始化
 * @note  在系统上电时调用，确保所有模式初始为安全状态
 */
void Robot_Global_Init(void) {
    // 结构体整体清零 (将所有浮点数置0，指针置空，新增的target_info也会被清零)
    memset(&global_info, 0, sizeof(global_info_t));

    Referee_Init();
    SuperCap_Init();

    // 关联裁判系统句柄
    global_info.referee = Referee_Get_Handle();
    global_info.super_cap = SuperCap_GetData();

}