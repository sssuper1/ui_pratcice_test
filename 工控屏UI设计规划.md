# 工控屏 UI 显示方案规划文档 (STM32 + FreeRTOS + OLED)

## 一、 系统架构与硬件约束 (Architecture & Constraints)
* **硬件平台**：`STM32F103`。核心限制在于 20KB SRAM，需严格控制 RTOS 任务栈大小与全局数组，禁止使用 `malloc` 动态内存。
* **外设接口**：OLED 显示屏 (SPI/I2C驱动) + 4x4 矩阵键盘控制。
* **操作系统**：FreeRTOS。彻底解耦 UI 渲染、按键扫描与串口通信。不使用庞大的 GUI 框架（如 LVGL），所有界面及菜单管理均基于 C 语言结合 RTOS 机制（队列、互斥锁）手写实现。

## 二、 核心任务分配 (RTOS Tasks)
基于 `projects/HMI_RTOS_Panel` 的架构，系统划分为以下核心任务：

### 1. 交互与显示任务 (`HMI_Task` & `Key_Task`)
* **优先级**：中等 (Normal)
* **职责**：
  - **4x4矩阵按键**：通过中断或定时轮询方式扫描矩阵键盘，完成防抖逻辑，并将键值（上下、确认、返回、数字输入等）发送至 UI 消息队列。
  - **OLED驱动刷新**：负责 OLED 屏幕局部的无闪烁刷新。
  - **多级菜单状态机**：运行 5 级链表/树状菜单逻辑（操作、功能、设置），解析用户的按键动作以切换当前页面或修改局部参数。
* **交互逻辑**：当用户在底层菜单完成参数修改，按下“确认”后，将此命令（参数地址+新值）压入 `UART_Msg_Queue` 供通信任务处理。

### 2. 串口通信与总线任务 (`UART_Comm_Task`)
* **优先级**：高 (High)
* **职责**：
  - **帧接收与解析**：利用 UART + DMA 不定长（Idle中断）机制被动接收主控板发来的 `0xD5 0x5D` 协议帧。主控板的 `write_ui_Thread` 会周期性（每秒多帧）主动下发 `0x04`、`0x05`、`0x06`、`0x07`、`0x08`、`0x09` 等指令。工控屏解包后提取出设备信息及运行各项指标（例如：信号强度、节点IP、频点、电量等）。
  - **状态更新 (被动刷新模式)**：提取到报文后，请求**“数据字典”的互斥锁 (`Mutex`)**，将解析到的新数据覆写进去。覆写完毕后释放锁，并向 UI 任务发送 Event 标志，通知按需重绘画布。
  - **数据下发 (主动回传模式)**：阻塞等待按键菜单传递来的 `UART_Msg_Queue`。一旦收到 HMI 任务传来的用户修改请求（如：操作模式更改、波特率设置等），立即组装上行包并发给主控板。

### 3. 系统监控守护任务 (`Monitor_Task` - 可选)
* **优先级**：低 (Low)
* **职责**：由于主控主动发送周期帧（04-09），且带有数据快照缓存保护，所以工控屏原则上不再需要高频发起**轮询查询式**通信。此任务可用作 RTOS 自身的硬件看门狗监控（喂狗、心跳LED闪烁）或者统计串口掉线超时。

## 三、 全局数据字典设计 (Data Dictionary)
为打破凌乱的变参，系统统一维护一个基于 `Mutex` 保护的数据字典：
```c
// 统一定义设备参数类型映射
typedef struct {
    uint16_t param_id;       // 参数在协议中的地址标识
    int32_t  current_value;  // 当前值
    int32_t  min_val;        // 最小值拦截
    int32_t  max_val;        // 最大值拦截
    uint8_t  permission;     // RO(只读展示) 或 RW(可读可写) 
} ParamDef_t;

// 全局字典与锁
extern ParamDef_t Global_Dict[];
// 无论 UART 任务更新状态，还是 HMI 任务读取状态，均需申请此 osMutexId_t
```

## 四、 5级菜单状态机设计 (Menu State Machine)
采用**节点链表树**结构构建菜单操作，涵盖**“操作”**、**“功能”**、**“设置”**三大主线。

### 1. 节点数据结构
```c
typedef struct MenuNode {
    uint8_t Menu_ID;
    struct MenuNode* Parent_ID;
    struct MenuNode* Child_ID;
    struct MenuNode* Sibling_ID;
    void (*Render_Callback)(void); // 该页面的 OLED 绘制函数
    void (*Key_Callback)(uint8_t key); // 该页面的 4x4 键盘处理逻辑
} MenuNode_t;
```

### 2. 菜单层级流转机制
* **第 1~4 级（导航级）**：
  - OLED 显示同级列表，选中项反白闪烁。
  - 矩阵键盘操作：上下键移动到 `Sibling_ID`，确认键进入 `Child_ID`，返回键跳转 `Parent_ID`。
* **第 5 级（叶子参数级，例如“波特率设置 / 调制方式设置”）**：
  - OLED 切换为带光标的编辑模式。
  - 矩阵键盘操作：用数字键或上下键改变值，调用数据字典中的 `min_val` 和 `max_val` 进行越界拦截。按下确认键后，不仅回写全局字典，同时投入 UART 发送队列以实际下发到设备。

## 五、 实施计划 (Roadmap)

### Day 1：BSP 驱动层落定
1. 跑通 FreeRTOS 系统 Tic 和时钟。
2. 实现裸机 `OLED_ShowString`、`OLED_DrawPoint`。
3. 调通 4x4 矩阵键盘扫描，封装出类似 `uint8_t Key_GetPressed()`，并通过 OS 消息队列发出事件。

### Day 2：通信解析与数据字典
1. 配置 UART+DMA (利用 Idle 中断) 实现无阻塞的 RingBuffer 接收。
2. 完成 `Global_Dict` 中各类设备核心状态参数的初始化。
3. 调通互斥量 `osMutexAcquire/Release`，并能在后台通过串口成功接收 `[0x08]` / `[0x05]` 包进行字典更新。

### Day 3~4：菜单树结构构建与整合
1. 录入所有菜单节点，包含“操作模式 -> 通道设置”、“节点详细信息”、“自检状态 -> 温度/计数”等。
2. 编写全局指针 `Current_Node`，将按键消息从键盘队列传给当前节点的 `Key_Callback`，并在结束后调用 `Render_Callback` 刷新 OLED 屏幕。
3. 对接底层数据字典，确保“功能”类菜单读取的均是锁保护下的最新后台数据。

## 六、 致命风险警告
1. **栈溢出 (Stack Overflow)**：20KB 内存跑这么多级菜单极易溢出。务必对 3 个主任务做精细的 Stack Size 规划（建议暂定 HMI=512 Words，UART_Comm=512 Words），并开启死机 Hook (`vApplicationStackOverflowHook`)。
2. **Flash 挤压**：OLED 汉字点阵极其占用 Flash（尤其是大量中文菜单词条）。建议采用动态提取的中文字库编译，或增加外部 SPI Flash(如 W25Q64) 存储字模库。
