//
// Created by 14717 on 2026/3/19.
//

#ifndef INFANTRY_01_REFEREE_H
#define INFANTRY_01_REFEREE_H

#include <stdint.h>

/* --- 协议帧常量定义 --- */
#define REFEREE_SOF             0xA5        // 帧起始标志
#define REFEREE_HEADER_SIZE     5           // 帧头大小（字节）
#define REFEREE_CMD_ID_SIZE     2           // 命令 ID 大小（字节）
#define REFEREE_TAIL_SIZE       2           // 帧尾 CRC16 大小（字节）
#define REFEREE_MAX_DATA_SIZE   256         // 数据部分最大长度

/* --- 裁判系统协议命令 ID 定义 --- */
#define CMD_ID_GAME_STATUS      0x0001      // 比赛状态信息
#define CMD_ID_ROBOT_HP         0x0003      // 机器人血量信息
#define CMD_ID_EVENT_DATA       0x0101      // 事件数据
#define CMD_ID_ROBOT_STATUS     0x0201      // 机器人状态信息
#define CMD_ID_POWER_HEAT_DATA  0x0202      // 功率和热量数据
#define CMD_ID_HURT_DATA        0x0206      // 受伤数据
#define CMD_ID_RFID_STATUS      0x0209      // RFID 增益点状态
#define CMD_ID_ROBOT_INTERACT   0x0301      // 机器人交互数据（含客户端绘图）

/* --- 交互子内容 ID --- */
#define REF_UI_DATA_ID_DELETE   0x0100
#define REF_UI_DATA_ID_DRAW_1   0x0101
#define REF_UI_DATA_ID_DRAW_2   0x0102
#define REF_UI_DATA_ID_DRAW_5   0x0103
#define REF_UI_DATA_ID_DRAW_7   0x0104
#define REF_UI_DATA_ID_CHAR     0x0110

/* --- 机器人交互数据长度上限（协议 x <= 112） --- */
#define REF_INTERACT_DATA_MAX_LEN 112U

/* --- 机器人 ID（官方定义） --- */
#define REF_ROBOT_ID_RED_HERO        1U
#define REF_ROBOT_ID_RED_ENGINEER    2U
#define REF_ROBOT_ID_RED_INFANTRY3   3U
#define REF_ROBOT_ID_RED_INFANTRY4   4U
#define REF_ROBOT_ID_RED_INFANTRY5   5U
#define REF_ROBOT_ID_RED_AERIAL      6U
#define REF_ROBOT_ID_RED_SENTRY      7U
#define REF_ROBOT_ID_RED_DART        8U
#define REF_ROBOT_ID_RED_RADAR       9U
#define REF_ROBOT_ID_RED_OUTPOST     10U
#define REF_ROBOT_ID_RED_BASE        11U

#define REF_ROBOT_ID_BLUE_HERO       101U
#define REF_ROBOT_ID_BLUE_ENGINEER   102U
#define REF_ROBOT_ID_BLUE_INFANTRY3  103U
#define REF_ROBOT_ID_BLUE_INFANTRY4  104U
#define REF_ROBOT_ID_BLUE_INFANTRY5  105U
#define REF_ROBOT_ID_BLUE_AERIAL     106U
#define REF_ROBOT_ID_BLUE_SENTRY     107U
#define REF_ROBOT_ID_BLUE_DART       108U
#define REF_ROBOT_ID_BLUE_RADAR      109U
#define REF_ROBOT_ID_BLUE_OUTPOST    110U
#define REF_ROBOT_ID_BLUE_BASE       111U

/* --- 常用接收者 ID --- */
#define REF_RECEIVER_ID_SERVER        0x8080U

/* --- 图形操作/类型/颜色枚举 --- */
typedef enum
{
    REF_UI_OP_NULL = 0,
    REF_UI_OP_ADD = 1,
    REF_UI_OP_MODIFY = 2,
    REF_UI_OP_DELETE = 3,
} referee_ui_op_t;

typedef enum
{
    REF_UI_TYPE_LINE = 0,
    REF_UI_TYPE_RECT = 1,
    REF_UI_TYPE_CIRCLE = 2,
    REF_UI_TYPE_ELLIPSE = 3,
    REF_UI_TYPE_ARC = 4,
    REF_UI_TYPE_FLOAT = 5,
    REF_UI_TYPE_INT = 6,
    REF_UI_TYPE_CHAR = 7,
} referee_ui_type_t;

typedef enum
{
    REF_UI_COLOR_SELF = 0,
    REF_UI_COLOR_YELLOW = 1,
    REF_UI_COLOR_GREEN = 2,
    REF_UI_COLOR_ORANGE = 3,
    REF_UI_COLOR_PURPLE = 4,
    REF_UI_COLOR_PINK = 5,
    REF_UI_COLOR_CYAN = 6,
    REF_UI_COLOR_BLACK = 7,
    REF_UI_COLOR_WHITE = 8,
} referee_ui_color_t;

/* --- DMA 双缓冲区配置 --- */
#define REFEREE_RX_BUF_NUM      512         // 单个缓冲区大小（根据实际最大帧长调整）

/* --- 裁判系统数据协议结构体声明 --- */
/**
 * @brief 比赛状态信息结构体 (总大小：11 字节)
 * 
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 1    | 比赛信息字节
 *            |      |   bit 0-3：比赛类型
 *            |      |     • 1：RoboMaster 机甲大师超级对抗赛
 *            |      |     • 2：RoboMaster 机甲大师高校单项赛
 *            |      |     • 3：ICRA RoboMaster 高校人工智能挑战赛
 *            |      |     • 4：RoboMaster 机甲大师高校联盟赛 3V3 对抗
 *            |      |     • 5：RoboMaster 机甲大师高校联盟赛步兵对抗
 *            |      |   bit 4-7：当前比赛阶段
 *            |      |     • 0：未开始比赛
 *            |      |     • 1：准备阶段
 *            |      |     • 2：十五秒裁判系统自检阶段
 *            |      |     • 3：五秒倒计时
 *            |      |     • 4：比赛中
 *            |      |     • 5：比赛结算中
 * 1          | 2    | 当前阶段剩余时间，单位：秒
 * 3          | 8    | UNIX 时间戳，当机器人正确连接到裁判系统的 NTP 服务器后生效
 */
typedef struct __attribute__((packed))
{
    uint8_t game_type : 4;        /**< 比赛类型 (bit 0-3) */
    uint8_t game_progress : 4;    /**< 当前比赛阶段 (bit 4-7) */
    uint16_t stage_remain_time;   /**< 当前阶段剩余时间 (秒) */
    uint64_t SyncTimeStamp;       /**< UNIX 时间戳 (NTP 同步后生效) */
} game_status_t;

/**
 * @brief 机器人血量信息结构体 (总大小：16 字节)
 * 
 * 协议 ID：0x0003
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 2    | 己方 1 号英雄机器人血量 (未上场或罚下时为 0)
 * 2          | 2    | 己方 2 号工程机器人血量
 * 4          | 2    | 己方 3 号步兵机器人血量
 * 6          | 2    | 己方 4 号步兵机器人血量
 * 8          | 2    | 保留位
 * 10         | 2    | 己方 7 号哨兵机器人血量
 * 12         | 2    | 己方前哨站血量
 * 14         | 2    | 己方基地血量
 */
typedef struct __attribute__((packed))
{
    uint16_t ally_1_robot_HP;     /**< 己方 1 号英雄机器人血量 */
    uint16_t ally_2_robot_HP;     /**< 己方 2 号工程机器人血量 */
    uint16_t ally_3_robot_HP;     /**< 己方 3 号步兵机器人血量 */
    uint16_t ally_4_robot_HP;     /**< 己方 4 号步兵机器人血量 */
    uint16_t reserved;            /**< 保留位 */
    uint16_t ally_7_robot_HP;     /**< 己方 7 号哨兵机器人血量 */
    uint16_t ally_outpost_HP;     /**< 己方前哨站血量 */
    uint16_t ally_base_HP;        /**< 己方基地血量 */
} game_robot_HP_t;

/**
 * @brief 事件数据信息结构体 (总大小：4 字节)
 * 
 * 协议 ID：0x0101
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 4    | 事件数据 (bit 0-31)
 *            |      |   bit 0-2：补给区占领状态
 *            |      |     • bit 0：己方与资源区区不重叠的补给区 (1=已占领)
 *            |      |     • bit 1：己方与资源区重叠的补给区 (1=已占领)
 *            |      |     • bit 2：己方补给区的占领状态 (1=已占领，仅 RMUL 适用)
 *            |      |
 *            |      |   bit 3-6：己方能量机关状态
 *            |      |     • bit 3-4：小能量机关 (0=未激活，1=已激活，2=正在激活)
 *            |      |     • bit 5-6：大能量机关 (0=未激活，1=已激活，2=正在激活)
 *            |      |
 *            |      |   bit 7-8：己方中央高地占领状态 (1=己方占领，2=对方占领)
 *            |      |   bit 9-10：己方梯形高地占领状态 (1=已占领)
 *            |      |
 *            |      |   bit 11-19：飞镖击中时间 (0-420 秒，开局默认 0)
 *            |      |   bit 20-22：飞镖击中目标类型
 *            |      |     • 0：默认值
 *            |      |     • 1：击中前哨站
 *            |      |     • 2：击中基地固定目标
 *            |      |     • 3：击中基地随机固定目标
 *            |      |     • 4：击中基地随机移动目标
 *            |      |     • 5：击中基地末端移动目标
 *            |      |
 *            |      |   bit 23-24：中心增益点占领状态 (仅 RMUL 适用)
 *            |      |     • 0：未被占领
 *            |      |     • 1：被己方占领
 *            |      |     • 2：被对方占领
 *            |      |     • 3：被双方占领
 *            |      |
 *            |      |   bit 25-26：己方堡垒增益点占领状态
 *            |      |     • 0：未被占领
 *            |      |     • 1：被己方占领
 *            |      |     • 2：被对方占领
 *            |      |     • 3：被双方占领
 *            |      |
 *            |      |   bit 27-28：己方前哨站增益点占领状态
 *            |      |     • 0：未被占领
 *            |      |     • 1：被己方占领
 *            |      |     • 2：被对方占领
 *            |      |
 *            |      |   bit 29：己方基地增益点占领状态 (1=已占领)
 *            |      |   bit 30-31：保留位
 */
typedef struct __attribute__((packed))
{
    uint32_t event_data;  /**< 事件数据 (包含所有占领状态和能量机关状态) */
} event_data_t;

/**
 * @brief 机器人状态信息结构体 (总大小：13 字节)
 * 
 * 协议 ID：0x0201
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 1    | 本机器人 ID
 * 1          | 1    | 机器人等级
 * 2          | 2    | 机器人当前血量
 * 4          | 2    | 机器人血量上限
 * 6          | 2    | 机器人射击热量每秒冷却值
 * 8          | 2    | 机器人射击热量上限
 * 10         | 2    | 机器人底盘功率上限
 * 12         | 1    | 电源管理模块输出状态
 *            |      |   bit 0：gimbal 口输出 (0=无输出，1=24V 输出)
 *            |      |   bit 1：chassis 口输出 (0=无输出，1=24V 输出)
 *            |      |   bit 2：shooter 口输出 (0=无输出，1=24V 输出)
 *            |      |   bit 3-7：保留位
 */
typedef struct __attribute__((packed))
{
    uint8_t robot_id;                       /**< 本机器人 ID */
    uint8_t robot_level;                    /**< 机器人等级 */
    uint16_t current_HP;                    /**< 机器人当前血量 */
    uint16_t maximum_HP;                    /**< 机器人血量上限 */
    uint16_t shooter_barrel_cooling_value;  /**< 射击热量每秒冷却值 */
    uint16_t shooter_barrel_heat_limit;     /**< 射击热量上限 */
    uint16_t chassis_power_limit;           /**< 底盘功率上限 */
    uint8_t power_management_gimbal_output : 1;   /**< gimbal 口输出 (bit 0) */
    uint8_t power_management_chassis_output : 1;  /**< chassis 口输出 (bit 1) */
    uint8_t power_management_shooter_output : 1;  /**< shooter 口输出 (bit 2) */
} robot_status_t;

/**
 * @brief 功率和热量数据信息结构体 (总大小：14 字节)
 * 
 * 协议 ID：0x0202
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 2    | 保留位
 * 2          | 2    | 保留位
 * 4          | 4    | 保留位
 * 8          | 2    | 缓冲能量（单位：J）
 * 10         | 2    | 17mm 发射机构的射击热量
 * 12         | 2    | 42mm 发射机构的射击热量
 */
typedef struct __attribute__((packed))
{
    uint16_t reserved_1;                    /**< 保留位 (字节 0-1) */
    uint16_t reserved_2;                    /**< 保留位 (字节 2-3) */
    float reserved_float;                   /**< 保留位 (字节 4-7) */
    uint16_t buffer_energy;                 /**< 缓冲能量 (单位：J，字节 8-9) */
    uint16_t shooter_17mm_barrel_heat;      /**< 17mm 发射机构射击热量 (字节 10-11) */
    uint16_t shooter_42mm_barrel_heat;      /**< 42mm 发射机构射击热量 (字节 12-13) */
} power_heat_data_t;

/**
 * @brief 受伤数据信息结构体 (总大小：1 字节)
 * 
 * 协议 ID：0x0203（假设）
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 1    | 受伤数据
 *            |      |   bit 0-3：装甲模块 ID
 *            |      |     • 当扣血原因为装甲模块被弹丸攻击、受撞击或离线时，
 *            |      |       为装甲模块或测速模块的 ID 编号
 *            |      |     • 当其他原因导致扣血时，该值为 0
 *            |      |
 *            |      |   bit 4-7：血量变化类型（扣血原因）
 *            |      |     • 0：装甲模块被弹丸攻击导致扣血
 *            |      |     • 1：装甲模块或超级电容管理模块离线导致扣血
 *            |      |     • 5：装甲模块受到撞击导致扣血
 */
typedef struct __attribute__((packed))
{
    uint8_t armor_id : 4;                   /**< 装甲模块 ID (bit 0-3) */
    uint8_t HP_deduction_reason : 4;        /**< 血量变化类型/扣血原因 (bit 4-7) */
} hurt_data_t;

/**
 * @brief RFID 增益点状态信息结构体 (总大小：5 字节)
 * 
 * 协议 ID：0x0209
 * 字节偏移量 | 大小 | 说明
 * -----------|------|------------------------------------------------
 * 0          | 4    | RFID 状态寄存器 1 (bit 0-31)
 *            |      |   bit 0：己方基地增益点
 *            |      |   bit 1：己方中央高地增益点
 *            |      |   bit 2：对方中央高地增益点
 *            |      |   bit 3：己方梯形高地增益点
 *            |      |   bit 4：对方梯形高地增益点
 *            |      |   bit 5：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡前）
 *            |      |   bit 6：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡后）
 *            |      |   bit 7：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡前）
 *            |      |   bit 8：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡后）
 *            |      |   bit 9：己方地形跨越增益点（中央高地下方）
 *            |      |   bit 10：己方地形跨越增益点（中央高地上方）
 *            |      |   bit 11：对方地形跨越增益点（中央高地下方）
 *            |      |   bit 12：对方地形跨越增益点（中央高地上方）
 *            |      |   bit 13：己方地形跨越增益点（公路下方）
 *            |      |   bit 14：己方地形跨越增益点（公路上方）
 *            |      |   bit 15：对方地形跨越增益点（公路下方）
 *            |      |   bit 16：对方地形跨越增益点（公路上方）
 *            |      |   bit 17：己方堡垒增益点
 *            |      |   bit 18：己方前哨站增益点
 *            |      |   bit 19：己方与资源区不重叠的补给区/RMUL 补给区
 *            |      |   bit 20：己方与资源区重叠的补给区
 *            |      |   bit 21：己方装配增益点
 *            |      |   bit 22：对方装配增益点
 *            |      |   bit 23：中心增益点（仅 RMUL 适用）
 *            |      |   bit 24：对方堡垒增益点
 *            |      |   bit 25：对方前哨站增益点
 *            |      |   bit 26：己方地形跨越增益点（隧道）（靠近己方一侧公路区下方）
 *            |      |   bit 27：己方地形跨越增益点（隧道）（靠近己方一侧公路区中间）
 *            |      |   bit 28：己方地形跨越增益点（隧道）（靠近己方一侧公路区上方）
 *            |      |   bit 29：己方地形跨越增益点（隧道）（靠近己方梯形高地较低处）
 *            |      |   bit 30：己方地形跨越增益点（隧道）（靠近己方梯形高地较中间）
 *            |      |   bit 31：己方地形跨越增益点（隧道）（靠近己方梯形高地较高处）
 *            |      |
 * 4          | 1    | RFID 状态寄存器 2 (bit 32-39)
 *            |      |   bit 0：对方地形跨越增益点（隧道）（靠近对方公路一侧下方）
 *            |      |   bit 1：对方地形跨越增益点（隧道）（靠近对方公路一侧中间）
 *            |      |   bit 2：对方地形跨越增益点（隧道）（靠近对方公路一侧上方）
 *            |      |   bit 3：对方地形跨越增益点（隧道）（靠近对方梯形高地较低处）
 *            |      |   bit 4：对方地形跨越增益点（隧道）（靠近对方梯形高地较中间）
 *            |      |   bit 5：对方地形跨越增益点（隧道）（靠近对方梯形高地较高处）
 *            |      |   bit 6-7：保留位
 * 
 * 注：所有 RFID 卡仅在赛内生效。在赛外，即使检测到对应的 RFID 卡，对应值也为 0。
 */
typedef struct __attribute__((packed))
{
    uint32_t rfid_status;       /**< RFID 状态寄存器 1 (bit 0-31，共 32 个增益点状态) */
    uint8_t rfid_status_2;      /**< RFID 状态寄存器 2 (bit 32-39，共 6 个隧道增益点状态) */
} rfid_status_t;

/* --- 客户端交互结构体（0x0301 载荷） --- */
typedef struct __attribute__((packed))
{
    uint16_t data_cmd_id;   /**< 子内容 ID，如 0x0101 */
    uint16_t sender_id;     /**< 发送者 robot_id */
    uint16_t receiver_id;   /**< 接收者 ID（客户端/机器人） */
} ext_student_interactive_header_t;

typedef struct __attribute__((packed))
{
    uint8_t delete_type;    /**< 0:空操作 1:删除图层 2:删除所有 */
    uint8_t layer;          /**< 图层 0~9 */
} interaction_layer_delete_t;

/* 图形参数描述（用于打包为 15 字节 interaction_figure） */
typedef struct
{
    uint8_t figure_name[3];
    uint8_t operate_type;   /**< 0:空 1:增加 2:修改 3:删除 */
    uint8_t figure_type;    /**< 0:直线 1:矩形 2:圆 3:椭圆 4:圆弧 5:浮点 6:整型 7:字符 */
    uint8_t layer;          /**< 0~9 */
    uint8_t color;          /**< 0~8 */
    uint16_t details_a;     /**< 9 bit */
    uint16_t details_b;     /**< 9 bit */
    uint16_t width;         /**< 10 bit */
    uint16_t start_x;       /**< 11 bit */
    uint16_t start_y;       /**< 11 bit */
    uint16_t details_c;     /**< 10 bit */
    uint16_t details_d;     /**< 11 bit */
    uint16_t details_e;     /**< 11 bit */
} interaction_figure_param_t;

/* --- 全局游戏信息聚合结构体 --- */
/**
 * @brief 全局游戏信息结构体（裁判系统数据总集合）
 * 
 * 包含所有从裁判系统接收的数据，用于机器人决策和控制。
 * 该结构体整合了比赛状态、血量信息、事件数据、机器人状态等全部协议数据。
 * 
 * 数据来源：通过 USART6 串口接收（波特率 115200，8N1）
 * 更新方式：DMA 双缓冲 + 空闲中断自动解析
 */
typedef struct game_info_t
{
    /* === 比赛基础信息 === */
    game_status_t game_status;                      /**< 比赛状态信息 */
    
    /* === 血量信息 === */
    game_robot_HP_t robot_hp;                       /**< 机器人血量信息 (协议 0x0003) */
    
    /* === 事件数据 === */
    event_data_t event_data;                        /**< 事件数据/战场状态 (协议 0x0101) */
    
    /* === 机器人自身状态 === */
    robot_status_t robot_status;                    /**< 机器人状态信息 (协议 0x0201) */
    power_heat_data_t power_heat_data;              /**< 功率和热量数据 (协议 0x0202) */
    hurt_data_t hurt_data;                          /**< 受伤数据 (协议 0x0203) */
    
    /* === RFID 增益点状态 === */
    rfid_status_t rfid_status;                      /**< RFID 增益点状态 (协议 0x0209) */

    /* === 诊断信息（用于排查热量不更新） === */
    uint32_t power_heat_last_update_tick;           /**< 0x0202 最近一次成功解析时刻(ms) */
    uint32_t power_heat_update_count;               /**< 0x0202 成功解析计数 */

} game_info_t;

/* --- 函数声明 --- */
/**
 * @brief 初始化裁判系统串口接收（使用 USART6，DMA 双缓冲 + 空闲中断）
 */
void Referee_Init(void);

/**
 * @brief 获取裁判系统信息句柄
 * @return game_info_t* 裁判系统信息指针（只包含裁判系统数据）
 */
const game_info_t* Referee_Get_Handle(void);

/**
 * @brief 通用 0x0301 机器人交互发送
 * @param data_cmd_id 子内容 ID（如 REF_UI_DATA_ID_DRAW_1）
 * @param sender_id 发送者 robot_id
 * @param receiver_id 接收者 ID（客户端/机器人）
 * @param data 交互数据段
 * @param data_len 交互数据段长度（<=112）
 * @return 1 发送成功, 0 失败
 */
uint8_t Referee_Send_Interactive(uint16_t data_cmd_id,
                                 uint16_t sender_id,
                                 uint16_t receiver_id,
                                 const uint8_t *data,
                                 uint16_t data_len);

/**
 * @brief 删除图层/全部图层（子内容 0x0100）
 */
uint8_t Referee_UI_Delete(uint16_t sender_id, uint16_t receiver_id, uint8_t delete_type, uint8_t layer);

/**
 * @brief 绘制一个图形（子内容 0x0101）
 */
uint8_t Referee_UI_Draw1(uint16_t sender_id, uint16_t receiver_id, const interaction_figure_param_t *figure);

/**
 * @brief 绘制两个图形（子内容 0x0102）
 */
uint8_t Referee_UI_Draw2(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[2]);

/**
 * @brief 绘制五个图形（子内容 0x0103）
 */
uint8_t Referee_UI_Draw5(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[5]);

/**
 * @brief 绘制七个图形（子内容 0x0104）
 */
uint8_t Referee_UI_Draw7(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[7]);

/**
 * @brief robot_id 转客户端 ID（常规规则：client_id = robot_id + 0x0100）
 */
uint16_t Referee_Get_ClientId_By_RobotId(uint16_t robot_id);

#endif //INFANTRY_01_REFEREE_H
