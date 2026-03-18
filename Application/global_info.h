#ifndef ROBOT_GLOBAL_H
#define ROBOT_GLOBAL_H

#include "struct_typedef.h"
#include "stdint.h"
#include "../Components/remote/remote.h"

typedef struct game_info_t game_info_t;
typedef struct super_capacitor_data_t super_capacitor_data_t;

/* --- 全局信息聚合结构体 --- */
/**
 * @brief 全局信息结构体（最高层级）
 * 
 * 整合所有数据源：
 * - 裁判系统串口数据（通过 UART 获取，引用 referee 模块的句柄）
 * - 超级电容管理系统数据（通过 CAN 总线获取）
 * 
 * 用于机器人全局决策和状态管理
 */
typedef struct
{
    const game_info_t* referee;                 /**< 裁判系统数据句柄（从 Referee_Get_Handle() 获取） */
    const super_capacitor_data_t* super_cap;    /**< 超级电容数据句柄（从 SuperCap_GetData() 获取） */

} global_info_t;

/* --- 全局变量声明 --- */
extern global_info_t global_info;

/* --- 核心工具函数 --- */
void Robot_Global_Init(void);

#endif
