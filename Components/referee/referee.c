//
// Created by 14717 on 2026/3/19.
//

#include "referee.h"
#include "usart.h"
#include "cmsis_os.h"
#include <string.h>

#include "../../Bsp/LED/bsp_LED.h"

/******************************************************************************************
 *                                   外设句柄声明（参考 VT13）
 ******************************************************************************************/
extern UART_HandleTypeDef huart6;      // 裁判系统 -> USART6
extern DMA_HandleTypeDef hdma_usart6_rx;

/******************************************************************************************
 *                                   静态变量定义（参考 VT13）
 ******************************************************************************************/
static uint8_t referee_rx_buf[2][REFEREE_RX_BUF_NUM];  // DMA 双缓冲区
static game_info_t referee_game_info;                  // 裁判系统信息（静态全局）
static uint8_t referee_tx_seq = 0U;                    // 发送帧序号

#define REF_UI_TX_TIMEOUT_MIN_MS   20U
#define REF_UI_TX_TIMEOUT_MAX_MS   80U
#define REF_UI_BAUD_BYTES_PER_MS   12U

/* --- CRC8 查找表 (RoboMaster 标准) --- */
const uint8_t crc8_table[256] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};

/* --- CRC16 查找表  --- */
const uint16_t crc16_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/**
 * @brief CRC8 单字节计算
 */
uint8_t Referee_CRC8_Calculate(uint8_t data, uint8_t crc) {
    return crc8_table[(crc ^ data) & 0xFF];
}

/**
 * @brief CRC8 多字节计算（用于帧头校验）
 */
uint8_t Referee_CRC8(const uint8_t *data, uint16_t len) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = Referee_CRC8_Calculate(data[i], crc);
    }
    return crc;
}

/**
 * @brief CRC16 计算（用于整包校验）
 */
uint16_t Referee_CRC16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ *data++) & 0x00FFU];
    }
    return crc;
}

/******************************************************************************************
 *                                   数据帧解析处理（参考 VT13）
 ******************************************************************************************/
static void Referee_Data_Parse(uint8_t *p_frame, uint16_t len)
{
    if (len < (REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE)) return;
    
    // 验证帧头起始字节
    if (p_frame[0] != REFEREE_SOF) return;

    
    // 验证帧头 CRC8
    uint8_t crc8_rx = p_frame[4];
    uint8_t crc8_calc = Referee_CRC8(p_frame, REFEREE_HEADER_SIZE - 1);
    if (crc8_rx != crc8_calc) return;
    
    // 提取数据长度和命令 ID
    uint16_t data_len = (uint16_t)p_frame[1] | ((uint16_t)p_frame[2] << 8);
    uint16_t cmd_id = (uint16_t)p_frame[5] | ((uint16_t)p_frame[6] << 8);

    // 验证帧尾 CRC16
    uint16_t frame_total = REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE + data_len + REFEREE_TAIL_SIZE;
    if (len < frame_total) return;
    
    uint16_t crc16_rx = (p_frame[frame_total - 1] << 8) | p_frame[frame_total - 2];
    uint16_t crc16_calc = Referee_CRC16(p_frame, frame_total - REFEREE_TAIL_SIZE);

    if (crc16_rx != crc16_calc) return;
    
    // CRC 校验通过，处理数据
    uint8_t *data_ptr = &p_frame[REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE];

    switch (cmd_id)
    {
        case CMD_ID_GAME_STATUS:
            if (data_len >= sizeof(game_status_t))
                memcpy(&referee_game_info.game_status, data_ptr, sizeof(game_status_t));
            break;
            
        case CMD_ID_ROBOT_HP:
            if (data_len >= sizeof(game_robot_HP_t))
                memcpy(&referee_game_info.robot_hp, data_ptr, sizeof(game_robot_HP_t));
            break;
            
        case CMD_ID_EVENT_DATA:
            if (data_len >= sizeof(event_data_t))
                memcpy(&referee_game_info.event_data, data_ptr, sizeof(event_data_t));
            break;
            
        case CMD_ID_ROBOT_STATUS:
            if (data_len >= sizeof(robot_status_t))
                memcpy(&referee_game_info.robot_status, data_ptr, sizeof(robot_status_t));
            break;
            
        case CMD_ID_POWER_HEAT_DATA:
            if (data_len >= sizeof(power_heat_data_t)) {
                memcpy(&referee_game_info.power_heat_data, data_ptr, sizeof(power_heat_data_t));
                referee_game_info.power_heat_last_update_tick = osKernelSysTick();
                referee_game_info.power_heat_update_count++;
            }
            break;
            
        case CMD_ID_HURT_DATA:
            if (data_len >= sizeof(hurt_data_t))
                memcpy(&referee_game_info.hurt_data, data_ptr, sizeof(hurt_data_t));
            break;
            
        case CMD_ID_RFID_STATUS:
            if (data_len >= sizeof(rfid_status_t))
                memcpy(&referee_game_info.rfid_status, data_ptr, sizeof(rfid_status_t));
            break;
            
        default:
            break;
    }
}

/******************************************************************************************
 *                                   初始化函数（参考 VT13 的 RC_Init）
 ******************************************************************************************/
void Referee_Init(void)
{
    // 清零裁判系统相关的数据
    memset(&referee_game_info, 0, sizeof(game_info_t));
    
    // 配置 USART6 使用 DMA 接收
    SET_BIT(huart6.Instance->CR3, USART_CR3_DMAR);  // 使能 DMA 接收
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);    // 使能空闲中断
    
    // 配置 DMA 双缓冲区
    __HAL_DMA_DISABLE(&hdma_usart6_rx);
    while(hdma_usart6_rx.Instance->CR & DMA_SxCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_usart6_rx);
    }
    
    hdma_usart6_rx.Instance->PAR = (uint32_t)&(USART6->DR);      // 外设地址
    hdma_usart6_rx.Instance->M0AR = (uint32_t)(referee_rx_buf[0]); // 内存 0 地址
    hdma_usart6_rx.Instance->M1AR = (uint32_t)(referee_rx_buf[1]); // 内存 1 地址
    hdma_usart6_rx.Instance->NDTR = REFEREE_RX_BUF_NUM;          // 数据长度
    
    SET_BIT(hdma_usart6_rx.Instance->CR, DMA_SxCR_DBM);  // 使能双缓冲模式
    SET_BIT(hdma_usart6_rx.Instance->CR, DMA_SxCR_CIRC); // 使能循环模式
    
    __HAL_DMA_ENABLE(&hdma_usart6_rx);
}

/******************************************************************************************
 *                                   USART6 串口中断服务函数（参考 VT13）
 ******************************************************************************************/
void USART6_IRQHandler(void)
{

    // 检查是否为空闲中断
    if (huart6.Instance->SR & UART_FLAG_IDLE)
    {


        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
        
        uint16_t rx_len;
        uint8_t current_mem = (hdma_usart6_rx.Instance->CR & DMA_SxCR_CT) ? 1 : 0;
        
        // 禁用 DMA，获取当前接收长度
        __HAL_DMA_DISABLE(&hdma_usart6_rx);
        rx_len = REFEREE_RX_BUF_NUM - hdma_usart6_rx.Instance->NDTR;
        hdma_usart6_rx.Instance->NDTR = REFEREE_RX_BUF_NUM;
        
        // 切换缓冲区
        if (current_mem == 0)
            SET_BIT(hdma_usart6_rx.Instance->CR, DMA_SxCR_CT);  // 切换到缓冲区 1
        else
            CLEAR_BIT(hdma_usart6_rx.Instance->CR, DMA_SxCR_CT); // 切换到缓冲区 0
        
        __HAL_DMA_ENABLE(&hdma_usart6_rx);
        
        // 处理接收到的数据（按帧解析）
        if (rx_len > 0)
        {
            uint8_t *p_buf = referee_rx_buf[current_mem];
            uint16_t offset = 0;


            while (offset < rx_len)
            {

                // 寻找帧头 0xA5
                if (p_buf[offset] != REFEREE_SOF)
                {
                    offset++;
                    continue;
                }

                // 计算本帧总长度
                if (offset + REFEREE_HEADER_SIZE > rx_len) break;
                
                uint16_t data_len = (uint16_t)p_buf[offset + 1] | ((uint16_t)p_buf[offset + 2] << 8);
                uint16_t frame_total = REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE + data_len + REFEREE_TAIL_SIZE;

                if (offset + frame_total > rx_len) break;
                
                // 解析本帧数据
                Referee_Data_Parse(&p_buf[offset], frame_total);
                offset += frame_total;
            }
        }
    }
}

/******************************************************************************************
 *                                   获取裁判系统数据句柄（参考 VT13 的 RC_Get_Handle）
 ******************************************************************************************/
const game_info_t* Referee_Get_Handle(void)
{
    return &referee_game_info;
}

/******************************************************************************************
 *                                   发送侧封装（USART6）
 ******************************************************************************************/
static void Referee_UI_Pack_Figure15(uint8_t out15[15], const interaction_figure_param_t *in)
{
    uint32_t cfg1;
    uint32_t cfg2;
    uint32_t cfg3;

    if ((out15 == NULL) || (in == NULL))
    {
        return;
    }

    out15[0] = in->figure_name[0];
    out15[1] = in->figure_name[1];
    out15[2] = in->figure_name[2];

    cfg1 = ((uint32_t)(in->operate_type & 0x07U)) |
           (((uint32_t)(in->figure_type & 0x07U)) << 3) |
           (((uint32_t)(in->layer & 0x0FU)) << 6) |
           (((uint32_t)(in->color & 0x0FU)) << 10) |
           (((uint32_t)(in->details_a & 0x01FFU)) << 14) |
           (((uint32_t)(in->details_b & 0x01FFU)) << 23);

    cfg2 = ((uint32_t)(in->width & 0x03FFU)) |
           (((uint32_t)(in->start_x & 0x07FFU)) << 10) |
           (((uint32_t)(in->start_y & 0x07FFU)) << 21);

    cfg3 = ((uint32_t)(in->details_c & 0x03FFU)) |
           (((uint32_t)(in->details_d & 0x07FFU)) << 10) |
           (((uint32_t)(in->details_e & 0x07FFU)) << 21);

    out15[3] = (uint8_t)(cfg1 & 0xFFU);
    out15[4] = (uint8_t)((cfg1 >> 8) & 0xFFU);
    out15[5] = (uint8_t)((cfg1 >> 16) & 0xFFU);
    out15[6] = (uint8_t)((cfg1 >> 24) & 0xFFU);

    out15[7] = (uint8_t)(cfg2 & 0xFFU);
    out15[8] = (uint8_t)((cfg2 >> 8) & 0xFFU);
    out15[9] = (uint8_t)((cfg2 >> 16) & 0xFFU);
    out15[10] = (uint8_t)((cfg2 >> 24) & 0xFFU);

    out15[11] = (uint8_t)(cfg3 & 0xFFU);
    out15[12] = (uint8_t)((cfg3 >> 8) & 0xFFU);
    out15[13] = (uint8_t)((cfg3 >> 16) & 0xFFU);
    out15[14] = (uint8_t)((cfg3 >> 24) & 0xFFU);
}

static uint16_t Referee_Pack_Frame(uint8_t *frame_buf,
                                   uint16_t cmd_id,
                                   const uint8_t *payload,
                                   uint16_t payload_len)
{
    uint16_t total_len;
    uint16_t crc16;

    if ((frame_buf == NULL) || ((payload == NULL) && (payload_len > 0U)))
    {
        return 0U;
    }

    if (payload_len > REFEREE_MAX_DATA_SIZE)
    {
        return 0U;
    }

    total_len = REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE + payload_len + REFEREE_TAIL_SIZE;

    frame_buf[0] = REFEREE_SOF;
    frame_buf[1] = (uint8_t)(payload_len & 0xFFU);
    frame_buf[2] = (uint8_t)((payload_len >> 8) & 0xFFU);
    frame_buf[3] = referee_tx_seq++;
    frame_buf[4] = Referee_CRC8(frame_buf, 4U);

    frame_buf[5] = (uint8_t)(cmd_id & 0xFFU);
    frame_buf[6] = (uint8_t)((cmd_id >> 8) & 0xFFU);

    if (payload_len > 0U)
    {
        memcpy(&frame_buf[7], payload, payload_len);
    }

    crc16 = Referee_CRC16(frame_buf, (uint16_t)(total_len - REFEREE_TAIL_SIZE));
    frame_buf[total_len - 2U] = (uint8_t)(crc16 & 0xFFU);
    frame_buf[total_len - 1U] = (uint8_t)((crc16 >> 8) & 0xFFU);

    return total_len;
}

uint16_t Referee_Get_ClientId_By_RobotId(uint16_t robot_id)
{
    switch (robot_id)
    {
        case REF_ROBOT_ID_RED_HERO:
        case REF_ROBOT_ID_RED_ENGINEER:
        case REF_ROBOT_ID_RED_INFANTRY3:
        case REF_ROBOT_ID_RED_INFANTRY4:
        case REF_ROBOT_ID_RED_INFANTRY5:
        case REF_ROBOT_ID_RED_AERIAL:
        case REF_ROBOT_ID_BLUE_HERO:
        case REF_ROBOT_ID_BLUE_ENGINEER:
        case REF_ROBOT_ID_BLUE_INFANTRY3:
        case REF_ROBOT_ID_BLUE_INFANTRY4:
        case REF_ROBOT_ID_BLUE_INFANTRY5:
        case REF_ROBOT_ID_BLUE_AERIAL:
            return (uint16_t)(robot_id + 0x0100U);

        default:
            return 0U; // 例如哨兵/雷达无对应选手端时返回无效 ID
    }
}

uint8_t Referee_Send_Interactive(uint16_t data_cmd_id,
                                 uint16_t sender_id,
                                 uint16_t receiver_id,
                                 const uint8_t *data,
                                 uint16_t data_len)
{
    uint8_t payload[6 + REF_INTERACT_DATA_MAX_LEN];
    uint8_t frame[REFEREE_HEADER_SIZE + REFEREE_CMD_ID_SIZE + 6 + REF_INTERACT_DATA_MAX_LEN + REFEREE_TAIL_SIZE];
    uint16_t payload_len;
    uint16_t frame_len;
    uint32_t tx_timeout_ms;
    HAL_StatusTypeDef ret;

    if ((data_len > 0U) && (data == NULL))
    {
        return 0U;
    }

    if (data_len > REF_INTERACT_DATA_MAX_LEN)
    {
        return 0U;
    }

    payload[0] = (uint8_t)(data_cmd_id & 0xFFU);
    payload[1] = (uint8_t)((data_cmd_id >> 8) & 0xFFU);
    payload[2] = (uint8_t)(sender_id & 0xFFU);
    payload[3] = (uint8_t)((sender_id >> 8) & 0xFFU);
    payload[4] = (uint8_t)(receiver_id & 0xFFU);
    payload[5] = (uint8_t)((receiver_id >> 8) & 0xFFU);

    if (data_len > 0U)
    {
        memcpy(&payload[6], data, data_len);
    }

    payload_len = (uint16_t)(6U + data_len);
    frame_len = Referee_Pack_Frame(frame, CMD_ID_ROBOT_INTERACT, payload, payload_len);
    if (frame_len == 0U)
    {
        return 0U;
    }

    tx_timeout_ms = ((uint32_t)frame_len / REF_UI_BAUD_BYTES_PER_MS) + 6U;
    if (tx_timeout_ms < REF_UI_TX_TIMEOUT_MIN_MS)
    {
        tx_timeout_ms = REF_UI_TX_TIMEOUT_MIN_MS;
    }
    if (tx_timeout_ms > REF_UI_TX_TIMEOUT_MAX_MS)
    {
        tx_timeout_ms = REF_UI_TX_TIMEOUT_MAX_MS;
    }

    ret = HAL_UART_Transmit(&huart6, frame, frame_len, tx_timeout_ms);
    return (ret == HAL_OK) ? 1U : 0U;
}

uint8_t Referee_UI_Delete(uint16_t sender_id, uint16_t receiver_id, uint8_t delete_type, uint8_t layer)
{
    interaction_layer_delete_t del;
    del.delete_type = delete_type;
    del.layer = layer;

    return Referee_Send_Interactive(REF_UI_DATA_ID_DELETE,
                                    sender_id,
                                    receiver_id,
                                    (const uint8_t *)&del,
                                    (uint16_t)sizeof(del));
}

uint8_t Referee_UI_Draw1(uint16_t sender_id, uint16_t receiver_id, const interaction_figure_param_t *figure)
{
    uint8_t packed[15];

    if (figure == NULL)
    {
        return 0U;
    }

    Referee_UI_Pack_Figure15(packed, figure);
    return Referee_Send_Interactive(REF_UI_DATA_ID_DRAW_1, sender_id, receiver_id, packed, sizeof(packed));
}

uint8_t Referee_UI_Draw2(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[2])
{
    uint8_t packed[30];

    if (figures == NULL)
    {
        return 0U;
    }

    Referee_UI_Pack_Figure15(&packed[0], &figures[0]);
    Referee_UI_Pack_Figure15(&packed[15], &figures[1]);

    return Referee_Send_Interactive(REF_UI_DATA_ID_DRAW_2, sender_id, receiver_id, packed, sizeof(packed));
}

uint8_t Referee_UI_Draw5(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[5])
{
    uint8_t packed[75];
    uint8_t i;

    if (figures == NULL)
    {
        return 0U;
    }

    for (i = 0U; i < 5U; i++)
    {
        Referee_UI_Pack_Figure15(&packed[i * 15U], &figures[i]);
    }

    return Referee_Send_Interactive(REF_UI_DATA_ID_DRAW_5, sender_id, receiver_id, packed, sizeof(packed));
}

uint8_t Referee_UI_Draw7(uint16_t sender_id,
                         uint16_t receiver_id,
                         const interaction_figure_param_t figures[7])
{
    uint8_t packed[105];
    uint8_t i;

    if (figures == NULL)
    {
        return 0U;
    }

    for (i = 0U; i < 7U; i++)
    {
        Referee_UI_Pack_Figure15(&packed[i * 15U], &figures[i]);
    }

    return Referee_Send_Interactive(REF_UI_DATA_ID_DRAW_7, sender_id, receiver_id, packed, sizeof(packed));
}

