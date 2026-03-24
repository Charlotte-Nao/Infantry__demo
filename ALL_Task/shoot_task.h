//
// Created by 14717 on 2026/3/19.
//

#ifndef INFANTRY_01_SHOOT_TASK_H
#define INFANTRY_01_SHOOT_TASK_H

typedef enum {
    STIR_JAM_IDLE = 0,    // 正常状态
    STIR_JAM_RECOVER,     // 反转退弹中
    STIR_JAM_COOLDOWN,    // 冷却停滞中
} stir_jam_state_e;

void shoot_task_func(void const * argument);

#endif //INFANTRY_01_SHOOT_TASK_H