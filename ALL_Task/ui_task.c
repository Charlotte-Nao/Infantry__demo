#include "../ALL_Task/ui_task.h"
#include "../Application/global_info.h"
#include "../Components/referee/referee.h"
#include "../Components/super_capacitor/super_capacitor.h"
#include "cmsis_os.h"
#include <math.h>
#include <string.h>

#define UI_SEND_PERIOD_MS            100U
#define UI_HIT_HIGHLIGHT_MS          300U
#define UI_CAP_VOLTAGE_MIN_X100      800U    //超级电容最低可达5V，在此选择8V用于保险起见
#define UI_CAP_VOLTAGE_MAX_X100      2750U  //超级电容最高可达到27.5V
#define UI_CAP_GREEN_X100            2000U
#define UI_CAP_YELLOW_X100           1500U
#define UI_CAP_MARK_15_X100          1500U
#define UI_CAP_MARK_20_X100          2000U
#define UI_POWER_MIN_X10             0
#define UI_POWER_MAX_X10             2000

// 圆弧状态区布局（以屏幕中心为圆心）
#define UI_CENTER_X                  960U
#define UI_CENTER_Y                  540U
// 两条都从底部中点(180deg)向两侧增长：功率向左，能量向右
#define UI_PWR_ARC_START_DEG         225U
#define UI_PWR_ARC_END_DEG           315U
#define UI_PWR_ARC_SPAN_DEG          (UI_PWR_ARC_END_DEG - UI_PWR_ARC_START_DEG)
#define UI_CAP_ARC_START_DEG         45U
#define UI_CAP_ARC_END_DEG           135U
#define UI_CAP_ARC_SPAN_DEG          (UI_CAP_ARC_END_DEG - UI_CAP_ARC_START_DEG)
#define UI_CAP_ARC_RADIUS            UI_PWR_ARC_RADIUS
#define UI_PWR_ARC_RADIUS            390U
#define UI_ARC_WIDTH                 12U
#define UI_HIT_ROTATE_SIGN           -1.0f

// 中间正下方装甲受击示意布局
#define UI_CAR_HALF_SIZE             60U
#define UI_CAR_CENTER_X              UI_CENTER_X
#define UI_CAR_CENTER_Y              (UI_CENTER_Y - 300U)
#define UI_CAR_X0                    (UI_CAR_CENTER_X - UI_CAR_HALF_SIZE)
#define UI_CAR_Y0                    (UI_CAR_CENTER_Y - UI_CAR_HALF_SIZE)
#define UI_CAR_X1                    (UI_CAR_CENTER_X + UI_CAR_HALF_SIZE)
#define UI_CAR_Y1                    (UI_CAR_CENTER_Y + UI_CAR_HALF_SIZE)

// 新增：弹道下坠瞄准参考点（实心正方形点）布局
#define UI_AIM_OFFSET_Y              60U    // 下方下坠补偿像素（增大该值，点向下移动）
#define UI_AIM_OFFSET_X              15U    // 向左偏移像素（增大该值，点向左移动；设为0则居中）
#define UI_AIM_X                     (UI_CENTER_X - UI_AIM_OFFSET_X)
#define UI_AIM_Y                     (UI_CENTER_Y - UI_AIM_OFFSET_Y) // Y轴原点在左下角，减去偏移量即为向下
#define UI_AIM_SIZE                  12U     // 正方形边长（即线宽与线长，数字越大点越粗）
#define UI_AIM_HALF_SIZE             (UI_AIM_SIZE / 2U)

typedef enum
{
    UI_SPIN_IDLE = 0,
    UI_SPIN_LEFT = 1,
    UI_SPIN_RIGHT = 2,
} ui_spin_state_t;

static uint16_t clamp_u16(int32_t v, uint16_t lo, uint16_t hi)
{
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (uint16_t)v;
}

static uint8_t armor_color_by_tick(uint32_t now, uint32_t last_hit_tick)
{
    return ((now - last_hit_tick) <= UI_HIT_HIGHLIGHT_MS) ? REF_UI_COLOR_ORANGE : REF_UI_COLOR_GREEN;
}

static uint8_t cap_color_by_voltage(uint16_t v_x100)
{
    if (v_x100 > UI_CAP_GREEN_X100) {
        return REF_UI_COLOR_GREEN;
    }
    if (v_x100 >= UI_CAP_YELLOW_X100) {
        return REF_UI_COLOR_YELLOW;
    }
    // 协议色表无纯红，使用橙色作为低电压告警色。
    return REF_UI_COLOR_ORANGE;
}

static int8_t armor_id_to_index(uint8_t armor_id)
{
    // 优先按0~3直映射；兼容极少数发送端把4当作右侧。
    if (armor_id <= 3U) return (int8_t)armor_id;
    if (armor_id == 4U) return 3;
    return -1;
}

// 自转指示暂不绘制，保留枚举定义避免影响其它模块引用。

static void make_rect(interaction_figure_param_t *fig,
                      const char name0,
                      const char name1,
                      const char name2,
                      uint8_t op,
                      uint8_t color,
                      uint16_t x0,
                      uint16_t y0,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t width)
{
    memset(fig, 0, sizeof(*fig));
    fig->figure_name[0] = (uint8_t)name0;
    fig->figure_name[1] = (uint8_t)name1;
    fig->figure_name[2] = (uint8_t)name2;
    fig->operate_type = op;
    fig->figure_type = REF_UI_TYPE_RECT;
    fig->layer = 0U;
    fig->color = color;
    fig->width = width;
    fig->start_x = x0;
    fig->start_y = y0;
    fig->details_d = x1;
    fig->details_e = y1;
}

static void make_line(interaction_figure_param_t *fig,
                      const char name0,
                      const char name1,
                      const char name2,
                      uint8_t op,
                      uint8_t color,
                      uint16_t x0,
                      uint16_t y0,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t width)
{
    memset(fig, 0, sizeof(*fig));
    fig->figure_name[0] = (uint8_t)name0;
    fig->figure_name[1] = (uint8_t)name1;
    fig->figure_name[2] = (uint8_t)name2;
    fig->operate_type = op;
    fig->figure_type = REF_UI_TYPE_LINE;
    fig->layer = 0U;
    fig->color = color;
    fig->width = width;
    fig->start_x = x0;
    fig->start_y = y0;
    fig->details_d = x1;
    fig->details_e = y1;
}

static void make_arc(interaction_figure_param_t *fig,
                     const char name0,
                     const char name1,
                     const char name2,
                     uint8_t op,
                     uint8_t color,
                     uint16_t center_x,
                     uint16_t center_y,
                     uint16_t radius,
                     uint16_t start_deg,
                     uint16_t end_deg,
                     uint16_t width)
{
    memset(fig, 0, sizeof(*fig));
    fig->figure_name[0] = (uint8_t)name0;
    fig->figure_name[1] = (uint8_t)name1;
    fig->figure_name[2] = (uint8_t)name2;
    fig->operate_type = op;
    fig->figure_type = REF_UI_TYPE_ARC;
    fig->layer = 0U;
    fig->color = color;
    fig->details_a = (uint16_t)(start_deg & 0x01FFU); // 起始角
    fig->details_b = (uint16_t)(end_deg & 0x01FFU);   // 终止角
    fig->width = width;
    fig->start_x = center_x;
    fig->start_y = center_y;
    fig->details_c = 0U;
    fig->details_d = radius; // x半轴
    fig->details_e = radius; // y半轴
}

static void rotate_local_line_to_screen(float x0_local, float y0_local,
                                        float x1_local, float y1_local,
                                        float theta,
                                        uint16_t cx, uint16_t cy,
                                        uint16_t *x0, uint16_t *y0,
                                        uint16_t *x1, uint16_t *y1)
{
    float c = cosf(theta);
    float s = sinf(theta);
    float rx0 = x0_local * c - y0_local * s;
    float ry0 = x0_local * s + y0_local * c;
    float rx1 = x1_local * c - y1_local * s;
    float ry1 = x1_local * s + y1_local * c;

    *x0 = clamp_u16((int32_t)((float)cx + rx0), 0U, 1920U);
    *y0 = clamp_u16((int32_t)((float)cy + ry0), 0U, 1080U);
    *x1 = clamp_u16((int32_t)((float)cx + rx1), 0U, 1920U);
    *y1 = clamp_u16((int32_t)((float)cy + ry1), 0U, 1080U);
}

void ui_task_func(void const * argument) {
    uint8_t cleared_once = 0U;
    uint8_t status_drawn_once = 0U;
    uint8_t armor_drawn_once = 0U;
    uint8_t send_selector = 0U;
    uint32_t last_send_tick = 0U;
    uint16_t last_sender_id = 0U;
    uint16_t last_receiver_id = 0U;
    uint8_t last_hurt_sig = 0U;
    uint16_t last_hp = 0xFFFFU;
    uint32_t armor_hit_tick[4] = {0U, 0U, 0U, 0U}; // 0前 1左 2后 3右
    float yaw_ui_zero = 0.0f;
    uint8_t yaw_ui_zero_inited = 0U;
    (void)argument;

    while (1) {
        const game_info_t *ref = global_info.referee;

        if (ref != NULL)
        {
            uint16_t sender_id = ref->robot_status.robot_id;
            uint16_t receiver_id;

            receiver_id = Referee_Get_ClientId_By_RobotId(sender_id);


            if ((sender_id != 0U) && (receiver_id != 0U))
            {
                // 当发送关系变化时，重新删除并创建图形
                if ((sender_id != last_sender_id) || (receiver_id != last_receiver_id))
                {
                    cleared_once = 0U;
                    status_drawn_once = 0U;
                    armor_drawn_once = 0U;
                    send_selector = 0U;
                    last_sender_id = sender_id;
                    last_receiver_id = receiver_id;
                }

                if (cleared_once == 0U)
                {
                    Referee_UI_Delete(sender_id, receiver_id, 2U, 0U); // 删除全部图层
                    cleared_once = 1U;
                    status_drawn_once = 0U;
                    armor_drawn_once = 0U;
                    send_selector = 0U;
                    last_send_tick = osKernelSysTick();
                }

                if ((osKernelSysTick() - last_send_tick) >= 100U)
                {
                    uint32_t now = osKernelSysTick();
                    uint8_t op_status = (status_drawn_once == 0U) ? REF_UI_OP_ADD : REF_UI_OP_MODIFY;
                    uint8_t op_armor = (armor_drawn_once == 0U) ? REF_UI_OP_ADD : REF_UI_OP_MODIFY;
                    uint8_t tx_ok;
                    interaction_figure_param_t status_figs7[7];
                    interaction_figure_param_t armor_figs7[7];

                    // 受击高亮：同时参考hurt信号变化与HP下降，避免重复命中同一面时不刷新。
                    {
                        uint8_t armor_id = ref->hurt_data.armor_id;
                        uint8_t reason = ref->hurt_data.HP_deduction_reason;
                        int8_t armor_idx = armor_id_to_index(armor_id);
                        uint8_t sig = (uint8_t)((reason << 4) | (armor_id & 0x0FU));
                        uint16_t hp_now = ref->robot_status.current_HP;
                        uint8_t hp_drop = 0U;

                        if ((last_hp != 0xFFFFU) && (hp_now < last_hp))
                        {
                            hp_drop = 1U;
                        }
                        last_hp = hp_now;

                        if (((sig != last_hurt_sig) || (hp_drop != 0U)) && (armor_idx >= 0))
                        {
                            armor_hit_tick[(uint8_t)armor_idx] = now;
                            last_hurt_sig = sig;
                        }
                    }

                    // 超级电容进度条（框 + 实际电量线）
                    {
                        int16_t v = (global_info.super_cap != NULL) ? global_info.super_cap->capacity_voltage : 0;
                        uint16_t v_clamp = clamp_u16((int32_t)v, (uint16_t)UI_CAP_VOLTAGE_MIN_X100, (uint16_t)UI_CAP_VOLTAGE_MAX_X100);
                        uint8_t cap_color = cap_color_by_voltage(v_clamp);
                        uint16_t v_range = (uint16_t)(UI_CAP_VOLTAGE_MAX_X100 - UI_CAP_VOLTAGE_MIN_X100);
                        uint16_t v_rel = (uint16_t)(v_clamp - UI_CAP_VOLTAGE_MIN_X100);
                        // 绿条按电量平方显示：fill ~ (v_rel / v_range)^2
                        uint16_t fill_deg = (uint16_t)(((uint64_t)v_rel * (uint64_t)v_rel * (uint64_t)UI_CAP_ARC_SPAN_DEG) /
                                                       ((uint64_t)v_range * (uint64_t)v_range));
                        uint16_t cap_start_deg;
                        uint16_t v15_clamp = clamp_u16((int32_t)UI_CAP_MARK_15_X100,
                                                       (uint16_t)UI_CAP_VOLTAGE_MIN_X100,
                                                       (uint16_t)UI_CAP_VOLTAGE_MAX_X100);
                        uint16_t v20_clamp = clamp_u16((int32_t)UI_CAP_MARK_20_X100,
                                                       (uint16_t)UI_CAP_VOLTAGE_MIN_X100,
                                                       (uint16_t)UI_CAP_VOLTAGE_MAX_X100);
                        uint16_t v15_rel = (uint16_t)(v15_clamp - UI_CAP_VOLTAGE_MIN_X100);
                        uint16_t v20_rel = (uint16_t)(v20_clamp - UI_CAP_VOLTAGE_MIN_X100);
                        uint16_t mark15_deg = (uint16_t)(UI_CAP_ARC_END_DEG -
                                            (uint16_t)(((uint64_t)v15_rel * (uint64_t)v15_rel * (uint64_t)UI_CAP_ARC_SPAN_DEG) /
                                                       ((uint64_t)v_range * (uint64_t)v_range)));
                        uint16_t mark20_deg = (uint16_t)(UI_CAP_ARC_END_DEG -
                                            (uint16_t)(((uint64_t)v20_rel * (uint64_t)v20_rel * (uint64_t)UI_CAP_ARC_SPAN_DEG) /
                                                       ((uint64_t)v_range * (uint64_t)v_range)));

                        // Referee角度定义: 0deg在12点方向、顺时针；换算到屏幕坐标需用(90-deg)
                        float rad15 = (90.0f - (float)mark15_deg) * 0.01745329252f;
                        float rad20 = (90.0f - (float)mark20_deg) * 0.01745329252f;
                        int32_t m15_x0 = (int32_t)UI_CENTER_X + (int32_t)((float)(UI_CAP_ARC_RADIUS - 10U) * cosf(rad15));
                        int32_t m15_y0 = (int32_t)UI_CENTER_Y + (int32_t)((float)(UI_CAP_ARC_RADIUS - 10U) * sinf(rad15));
                        int32_t m15_x1 = (int32_t)UI_CENTER_X + (int32_t)((float)(UI_CAP_ARC_RADIUS + 10U) * cosf(rad15));
                        int32_t m15_y1 = (int32_t)UI_CENTER_Y + (int32_t)((float)(UI_CAP_ARC_RADIUS + 10U) * sinf(rad15));
                        int32_t m20_x0 = (int32_t)UI_CENTER_X + (int32_t)((float)(UI_CAP_ARC_RADIUS - 10U) * cosf(rad20));
                        int32_t m20_y0 = (int32_t)UI_CENTER_Y + (int32_t)((float)(UI_CAP_ARC_RADIUS - 10U) * sinf(rad20));
                        int32_t m20_x1 = (int32_t)UI_CENTER_X + (int32_t)((float)(UI_CAP_ARC_RADIUS + 10U) * cosf(rad20));
                        int32_t m20_y1 = (int32_t)UI_CENTER_Y + (int32_t)((float)(UI_CAP_ARC_RADIUS + 10U) * sinf(rad20));

                        uint16_t mark15_x0 = clamp_u16(m15_x0, 0U, 1920U);
                        uint16_t mark15_y0 = clamp_u16(m15_y0, 0U, 1080U);
                        uint16_t mark15_x1 = clamp_u16(m15_x1, 0U, 1920U);
                        uint16_t mark15_y1 = clamp_u16(m15_y1, 0U, 1080U);
                        uint16_t mark20_x0 = clamp_u16(m20_x0, 0U, 1920U);
                        uint16_t mark20_y0 = clamp_u16(m20_y0, 0U, 1080U);
                        uint16_t mark20_x1 = clamp_u16(m20_x1, 0U, 1920U);
                        uint16_t mark20_y1 = clamp_u16(m20_y1, 0U, 1080U);

                        cap_start_deg = (uint16_t)(UI_CAP_ARC_END_DEG - fill_deg);

                        if ((v_clamp > UI_CAP_VOLTAGE_MIN_X100) && (fill_deg == 0U)) {
                            fill_deg = 1U;
                            cap_start_deg = (uint16_t)(UI_CAP_ARC_END_DEG - fill_deg);
                        }
                        if (fill_deg > UI_CAP_ARC_SPAN_DEG) {
                            fill_deg = UI_CAP_ARC_SPAN_DEG;
                            cap_start_deg = UI_CAP_ARC_START_DEG;
                        }

                        make_arc(&status_figs7[0], 'C', 'B', '0', op_status, REF_UI_COLOR_WHITE,
                                 UI_CENTER_X, UI_CENTER_Y, UI_CAP_ARC_RADIUS,
                                 UI_CAP_ARC_START_DEG, UI_CAP_ARC_END_DEG, 2U);
                        make_arc(&status_figs7[1], 'C', 'B', '1', op_status, cap_color,
                                 UI_CENTER_X, UI_CENTER_Y, UI_CAP_ARC_RADIUS,
                                 cap_start_deg, UI_CAP_ARC_END_DEG, UI_ARC_WIDTH);
                        make_line(&status_figs7[2], 'C', 'B', '2', op_status, REF_UI_COLOR_YELLOW,
                                  mark15_x0, mark15_y0, mark15_x1, mark15_y1, 2U);
                        make_line(&status_figs7[3], 'C', 'B', '3', op_status, REF_UI_COLOR_GREEN,
                                  mark20_x0, mark20_y0, mark20_x1, mark20_y1, 2U);
                    }

                    // 底盘实时功率（仅条形）
                    {
                        int32_t p_x10 = 0;
                        uint16_t p_clamp;
                        uint16_t fill_deg;
                        uint16_t pwr_end_deg;

                        if (global_info.super_cap != NULL)
                        {
                            p_x10 = (int32_t)global_info.super_cap->chassis_output_power;
                        }

                        // 当超级电容链路暂时无有效值时，回退使用裁判系统底盘功率(float W)
                        if ((p_x10 == 0) && (ref != NULL))
                        {
                            p_x10 = (int32_t)(ref->power_heat_data.reserved_float * 10.0f);
                        }

                        // 统一显示功率幅值，避免发送端符号约定不同导致条形恒为0
                        if (p_x10 < 0)
                        {
                            p_x10 = -p_x10;
                        }

                        p_clamp = clamp_u16(p_x10, (uint16_t)UI_POWER_MIN_X10, (uint16_t)UI_POWER_MAX_X10);
                        fill_deg = (uint16_t)(((uint32_t)(p_clamp - UI_POWER_MIN_X10) * UI_PWR_ARC_SPAN_DEG) /
                                              (uint32_t)(UI_POWER_MAX_X10 - UI_POWER_MIN_X10));
                        pwr_end_deg = (uint16_t)(UI_PWR_ARC_START_DEG + fill_deg);
                        if ((p_clamp > 0U) && (fill_deg == 0U))
                        {
                            fill_deg = 1U;
                            pwr_end_deg = (uint16_t)(UI_PWR_ARC_START_DEG + fill_deg);
                        }
                        if (fill_deg > UI_PWR_ARC_SPAN_DEG)
                        {
                            fill_deg = UI_PWR_ARC_SPAN_DEG;
                            pwr_end_deg = UI_PWR_ARC_END_DEG;
                        }

                        make_arc(&status_figs7[4], 'P', 'W', '0', op_status, REF_UI_COLOR_WHITE,
                                 UI_CENTER_X, UI_CENTER_Y, UI_PWR_ARC_RADIUS,
                                 UI_PWR_ARC_START_DEG, UI_PWR_ARC_END_DEG, 2U);
                        make_arc(&status_figs7[5], 'P', 'W', '1', op_status, REF_UI_COLOR_YELLOW,
                                 UI_CENTER_X, UI_CENTER_Y, UI_PWR_ARC_RADIUS,
                                 UI_PWR_ARC_START_DEG, pwr_end_deg, UI_ARC_WIDTH);
                        // 中间车体方框固定显示（不随旋转）。
                        make_rect(&status_figs7[6], 'C', 'M', '0', op_status, REF_UI_COLOR_WHITE,
                                  UI_CAR_X0, UI_CAR_Y0, UI_CAR_X1, UI_CAR_Y1, 3U);
                    }

                    // 小车俯视图 + 四面装甲高亮
                    {
                    uint16_t car_cx = (uint16_t)((UI_CAR_X0 + UI_CAR_X1) / 2U);
                    uint16_t car_cy = (uint16_t)((UI_CAR_Y0 + UI_CAR_Y1) / 2U);
                    uint16_t half_w = (uint16_t)((UI_CAR_X1 - UI_CAR_X0) / 2U);
                    uint16_t half_h = (uint16_t)((UI_CAR_Y1 - UI_CAR_Y0) / 2U);
                    uint16_t out = 14U;
                    float yaw_pos = 0.0f;
                    float theta = 0.0f;
                    uint16_t ax0, ay0, ax1, ay1;

                    if (SuperCap_GetYawPosRad(&yaw_pos) != 0U)
                    {
                        if (yaw_ui_zero_inited == 0U)
                        {
                            yaw_ui_zero = yaw_pos;
                            yaw_ui_zero_inited = 1U;
                        }
                        theta = UI_HIT_ROTATE_SIGN * (yaw_pos - yaw_ui_zero);
                    }

                    // 小装甲板改为粗线，避免小矩形在客户端不渲染。
                    rotate_local_line_to_screen(-26.0f, (float)half_h + (float)out,
                                                26.0f, (float)half_h + (float)out,
                                                theta, car_cx, car_cy, &ax0, &ay0, &ax1, &ay1);
                    make_line(&armor_figs7[0], 'A', 'F', '0', op_armor, armor_color_by_tick(now, armor_hit_tick[0]),
                              ax0, ay0, ax1, ay1, 10U); // 前

                    rotate_local_line_to_screen(-(float)half_w - (float)out, -26.0f,
                                                -(float)half_w - (float)out, 26.0f,
                                                theta, car_cx, car_cy, &ax0, &ay0, &ax1, &ay1);
                    make_line(&armor_figs7[1], 'A', 'L', '0', op_armor, armor_color_by_tick(now, armor_hit_tick[1]),
                              ax0, ay0, ax1, ay1, 10U); // 左

                    rotate_local_line_to_screen(-26.0f, -(float)half_h - (float)out,
                                                26.0f, -(float)half_h - (float)out,
                                                theta, car_cx, car_cy, &ax0, &ay0, &ax1, &ay1);
                    make_line(&armor_figs7[2], 'A', 'B', '0', op_armor, armor_color_by_tick(now, armor_hit_tick[2]),
                              ax0, ay0, ax1, ay1, 10U); // 后

                    rotate_local_line_to_screen((float)half_w + (float)out, -26.0f,
                                                (float)half_w + (float)out, 26.0f,
                                                theta, car_cx, car_cy, &ax0, &ay0, &ax1, &ay1);
                    make_line(&armor_figs7[3], 'A', 'R', '0', op_armor, armor_color_by_tick(now, armor_hit_tick[3]),
                              ax0, ay0, ax1, ay1, 10U); // 右

                    // 屏幕下方前进方向辅助线（左右对称）
                    make_line(&armor_figs7[4], 'G', 'D', 'L', op_armor, REF_UI_COLOR_CYAN,
                              (uint16_t)(UI_CENTER_X - 480U), 20U,
                              (uint16_t)(UI_CENTER_X - 240U), (uint16_t)(UI_CAR_Y0 + 160U), 2U);
                    make_line(&armor_figs7[5], 'G', 'D', 'R', op_armor, REF_UI_COLOR_CYAN,
                              (uint16_t)(UI_CENTER_X + 480U), 20U,
                              (uint16_t)(UI_CENTER_X + 240U), (uint16_t)(UI_CAR_Y0 + 160U), 2U);

                        // 弹道下坠瞄准参考点（利用长度等于线宽的线段，画出一个实心正方形）
                        // 弹道下坠瞄准参考点（利用长度等于线宽的线段，画出一个实心正方形）
                    make_line(&armor_figs7[6], 'A', 'I', 'M', op_armor, REF_UI_COLOR_PINK,
                            (uint16_t)(UI_AIM_X - UI_AIM_HALF_SIZE), UI_AIM_Y,
                            (uint16_t)(UI_AIM_X + UI_AIM_HALF_SIZE), UI_AIM_Y, UI_AIM_SIZE);

                    }

                    if (send_selector == 0U)
                    {
                        tx_ok = Referee_UI_Draw7(sender_id, receiver_id, status_figs7);
                        if (tx_ok != 0U) status_drawn_once = 1U;
                    }
                    else if (send_selector == 1U)
                    {
                        tx_ok = Referee_UI_Draw7(sender_id, receiver_id, armor_figs7);
                        if (tx_ok != 0U) armor_drawn_once = 1U;
                    }

                    send_selector ^= 1U;
                    last_send_tick = osKernelSysTick();
                }
            }
        }

        osDelay(2);
    }
}

