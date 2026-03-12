#include <stdio.h>
#include <stdint.h>

// 1. 完美复用原项目中的 CRC 校验算法
uint16_t CRC_Check(uint8_t *CRC_Ptr, uint16_t LEN)
{
    uint16_t CRC_Value = 0xffff;
    for (int i = 0; i < LEN; i++)
    {
        CRC_Value ^= CRC_Ptr[i];
        for (int j = 0; j < 8; j++)
        {
            if (CRC_Value & 0x0001)
                CRC_Value = (CRC_Value >> 1) ^ 0xA001;
            else
                CRC_Value = (CRC_Value >> 1);
        }
    }
    // 原代码自带高低位反转
    CRC_Value = ((CRC_Value >> 8) | (CRC_Value << 8));
    return CRC_Value;
}

// 打印十六进制数组
void print_hex(uint8_t *buf, int len, const char *msg)
{
    printf("%s: \n", msg);
    for (int i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\n\n");
}

// 2. 生成 1字节 参数配置帧 (适用于 空域滤波、跳频方式、工作模式等)
void generate_1byte_frame(uint32_t addr, uint8_t value)
{
    uint8_t frame[14];
    
    frame[0] = 0xD5;         // 帧头
    frame[1] = 0x5D;
    frame[2] = 0x01;         // 命令字 (可填非0x0A的任意值，如0x01)
    frame[3] = 0xFF;         // 应答标志 (主动发出)
    frame[4] = 0x06;         // 长度 (4字节地址 + 1字节值 + 1)
    
    // 地址部分 (大端模式转换)
    frame[5] = (addr >> 24) & 0xFF;
    frame[6] = (addr >> 16) & 0xFF;
    frame[7] = (addr >> 8)  & 0xFF;
    frame[8] = addr & 0xFF;
    
    // 参数值
    frame[9] = value;
    
    // 计算 CRC (从第2字节[命令字]开始，到参数值[第9字节]结束，长度为8)
    uint16_t crc = CRC_Check(&frame[2], 8);
    
    // 填入 CRC (模拟 htons 效果，取高位在前，低位在后)
    frame[10] = (crc >> 8) & 0xFF;
    frame[11] = crc & 0xFF;
    
    // 帧尾
    frame[12] = 0x5D;
    frame[13] = 0xD5;
    
    print_hex(frame, sizeof(frame), "=> [1 Byte] 下发数据帧");
}

// 3. 生成 4字节 参数配置帧 (适用于 点频频率、自适应选频频率等)
void generate_4byte_frame(uint32_t addr, uint32_t value)
{
    uint8_t frame[17];
    
    frame[0] = 0xD5;
    frame[1] = 0x5D;
    frame[2] = 0x01;         // 命令字
    frame[3] = 0xFF;         // 应答标志
    frame[4] = 0x09;         // 长度 (4字节地址 + 4字节值 + 1)
    
    // 地址部分 (大端)
    frame[5] = (addr >> 24) & 0xFF;
    frame[6] = (addr >> 16) & 0xFF;
    frame[7] = (addr >> 8)  & 0xFF;
    frame[8] = addr & 0xFF;
    
    // 参数值 (大端)
    frame[9]  = (value >> 24) & 0xFF;
    frame[10] = (value >> 16) & 0xFF;
    frame[11] = (value >> 8)  & 0xFF;
    frame[12] = value & 0xFF;
    
    // 计算 CRC (长度为11)
    uint16_t crc = CRC_Check(&frame[2], 11);
    
    frame[13] = (crc >> 8) & 0xFF;
    frame[14] = crc & 0xFF;
    
    frame[15] = 0x5D;
    frame[16] = 0xD5;
    
    print_hex(frame, sizeof(frame), "=> [4 Byte] 下发数据帧");
}

// 4. 生成 0x0A 特殊请求帧 (查询目标节点详情)
void generate_0a_frame(uint8_t target_node_id)
{
    uint8_t frame[9];
    frame[0] = 0xD5;
    frame[1] = 0x5D;
    frame[2] = 0x0A;         // 必须为 0x0A
    frame[3] = 0xFF;         // 应答标志
    frame[4] = target_node_id; // Length字段被用来存放 Node ID
    
    // 计算 CRC (长度为3: 0A FF NodeID)
    uint16_t crc = CRC_Check(&frame[2], 3);
    
    frame[5] = (crc >> 8) & 0xFF;
    frame[6] = crc & 0xFF;
    
    frame[7] = 0x5D;
    frame[8] = 0xD5;
    
    print_hex(frame, sizeof(frame), "=> [0x0A 请求] 下发数据帧");
}

int main()
{
    printf("---------- 串口工控屏模拟指令生成器 ----------\n\n");
    
    // 测试案例 1: 开启空域滤波 (1字节修改)
    // 宏: PARAM_OP_MODE_SPATIAL_FILTERING 0x11300000 
    printf("[测试 1]: 修改空域滤波为 开启(0x00)\n");
    generate_1byte_frame(0x11300000, 0x00);
    
    // 测试案例 2: 修改路由协议为 BATMAN (1字节修改)
    // 宏: PARAM_CH1_ROUTING_PROTOCOL 0x12310000
    printf("[测试 2]: 修改路由协议为 BATMAN(0x02)\n");
    generate_1byte_frame(0x12310000, 0x02);
    
    // 测试案例 3: 修改点频中心频率为 500MHz (4字节修改)
    // 宏: PARAM_CH1_FIXED_FREQ_CENTER 0x12351000
    // 注意: 代码中会对该值除以1000，因此下发值需填入 500 * 1000 = 500000
    printf("[测试 3]: 修改点频中心频率为 500MHz (写入值 500000)\n");
    generate_4byte_frame(0x12351000, 500000);
    
    // 测试案例 4: 查询 Node 5 的节点详细信息
    // 宏: PARAM_0A_REQUEST_ADDR 0x10000000 (但0x0A指令会直接忽略地址)
    printf("[测试 4]: 请求 Node 5 节点详细信息\n");
    generate_0a_frame(0x05);

    return 0;
}