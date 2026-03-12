# 项目总结（用于简历与技术面试）

## 一、项目定位（大框架）
- 项目名称：IMX6ULL 工控屏/网管/WEB 一体化通信与展示系统（含串口协议、DB 数据桥接、仿真心跳）
- 运行环境：IMX6ULL Linux + C（GNU99）
- 本文档聚焦范围（你在简历里可直接声明）：
  - 涉及：工控屏串口协议（`/dev/ttymxc2`）、参数下发/ACK、周期上报帧构造、SQLite 数据同步、仿真心跳、网管侧消息接入与分发
  - 不展开：虚拟网卡/内核侧实现细节（虽通过 netlink 接入），以及 `www/` 前端实现细节（前端保持不改动）

## 二、系统目标（从需求到落地）
- 目标1：让工控屏可以“像真实设备一样”下发参数、收到 ACK，并持续看到设备状态上报
- 目标2：让 Web 网管页面在“不改动前端”的前提下自动刷新数据（通过 DB 驱动）
- 目标3：无真实硬件/底层不可用时，仍可通过仿真层走通端到端链路，便于演示与测试

## 三、模块划分（架构视图）
```mermaid
flowchart LR
  subgraph UI[工控屏串口层 /dev/ttymxc2]
    UIR[读线程: get_ui_Thread] -->|命令帧 0x01/0x0A| PARSE[process_uart_info]
    UIW[写线程: write_ui_Thread] -->|上报帧 0x04~0x09| UARTTX((UART TX))
  end

  subgraph CORE[中控业务层]
    PARSE --> EXEC[process_cmd_info\n参数映射/落盘/下发]
    EXEC --> NL[mgmt_netlink_set_param\n(对接内核/虚拟网卡层)]
    NL --> SIM[sim_heartbeat\n(无底层时仿真)]
  end

  subgraph DB[SQLite 数据层]
    EXEC --> DBW[updateData_* + persist_test_db]
    MGMT[mgmt_get_msg/mgmt_recv_msg] --> DBW
  end

  subgraph WEB[Web 展示层（不改 www/）]
    DBW --> WEBR[/www/cgi/test.db\n前端读取/]
  end
```

## 四、功能清单（高层）
- 串口通讯（工控屏）：
  - 命令解析与执行：写参数（0x01）、节点详情触发（0x0A）
  - 参数下发：频率/带宽/MCS/路由/工作模式/同步模式/统计控制等
  - 周期性状态上报：0x04~0x09（运行参数、设备信息、自检、邻居拓扑等）
  - 可靠性：CRC16 校验 + ACK（0x02）闭环
- 网管通讯：
  - 多端口 select 监听：控制口/管理口/广播口/网关口/TCP 客户端
  - 拓扑信息发送、转发与成员详情请求处理
  - 参数下发与系统操作入口（重启/关机/恢复出厂等）
- 数据持久化与 Web 展示：
  - SQLite 多表写入（systemInfo/meshInfo/link/timeslot 等）
  - “不改前端”策略：通过 `persist_test_db()` 将 DB 安全复制到 Web 读取路径
- 仿真层：
  - 无底层硬件时注入“心跳+状态”，同步 FREQ/BW/MCS/POWER 等全局初值，保证 Web 和串口上报一致

## 五、关键技术点（面试可讲）
- 串口协议工程化：自定义帧格式、CRC16 校验、ACK 确认、大小端一致性、读写线程解耦
- 数据一致性：工控屏下发参数不仅落盘（`/etc/node_xwg`），还同步写入 SQLite 并持久化到 Web 可读 DB
- 可靠持久化：`copy + fsync + rename` 替换 DB，避免半写入导致前端读到损坏文件
- 可演示可测试：仿真层替代底层硬件，使整链路可在面试/演示环境稳定运行

## 六、代码结构与关键文件
- 语言 & 平台：C（GNU99）、IMX6ULL Linux
- 串口：termios 115200/8N1，无流控
- DB：SQLite（WAL + FULL synchronous）
- 通信：UDP/TCP socket、Netlink（不展开 veth 内核侧）
- 关键文件（按“你的工作重点”排序）：
  - 工控屏串口协议与命令处理：[ui_get.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c)、[ui_get.h](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.h)
  - 数据库读写与持久化：[sqlite_unit.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sqlite_unit.c)
  - 网管消息循环与分发：[mgmt_transmit.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/mgmt_transmit.c)
  - netlink 接口层（对接内核/仿真）：[mgmt_netlink.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/mgmt_netlink.c)
  - 仿真心跳层：[sim_heartbeat.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sim_heartbeat.c)
  - 主程序入口与线程创建：[main.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/main.c)

## 七、线程模型与职责（落地细节）
线程创建入口：[main.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/main.c#L34-L55)

- 状态上报线程：`mgmt_get_msg`（写库为主）
  - 定时获取网络/节点信息（通过 netlink 或仿真层），刷新 `systemInfo/link/timeslot` 等表
- 网管接收线程：`mgmt_recv_web`（网管→设备下发入口之一）
  - 接收网管参数设置消息并调用 `mgmt_netlink_set_param` 下发
- 网管消息线程：`mgmt_recv_msg`（select 多端口事件循环）
  - 同时监听多 UDP/TCP 端口：广播参数、拓扑、控制、成员详情请求、文件更新等
- DB 参数下发线程：`sqlite_set_param`（Web→设备下发入口之二）
  - 轮询 SQLite 中 state=1 的参数变更，构造 `Smgmt_set_param` 下发并持久化 DB
- GPS线程：`gps_Thread`
  - 读取 GPS NMEA（GNGGA）解析时间与经纬度；若无设备则自动使用假数据回退（便于演示）
- 工控屏串口读线程：`get_ui_Thread`
  - 读取 `/dev/ttymxc2`，解析工控屏写命令（0x01/0x0A），执行后回 ACK
- 工控屏串口写线程：`write_ui_Thread`
  - 周期性向工控屏上报设备状态帧（0x04~0x09）

## 八、端到端数据流（你面试时的主线）
### 8.1 上行（设备→工控屏）
1. `write_ui_Thread` 从本地配置与运行状态读取参数
2. 构造上报帧：0x04/0x05/0x06/0x07/0x08/0x09
3. 写入 `/dev/ttymxc2`，工控屏侧解析展示

### 8.2 上行（设备→SQLite→Web）
1. `mgmt_get_msg` 周期采集运行数据（netlink/仿真）并写入 SQLite
2. `persist_test_db()` 将 DB 安全复制到 Web 读取路径（前端不改动）
3. Web 页面从 DB 读取刷新展示

### 8.3 下行（工控屏→设备→ACK/DB）
1. 工控屏下发 0x01/0x0A 帧 → `/dev/ttymxc2`
2. `get_ui_info` 读串口 → `process_uart_info` 校验头/尾/CRC
3. `process_cmd_info` 做“地址→业务参数”的映射，并：
   - 需要下发的：组装 `Smgmt_set_param` → `mgmt_netlink_set_param`（对接内核/仿真）
   - 需要展示的：`updateData_systeminfo_qk` 等写 SQLite，并 `persist_test_db()`
4. 组装 ACK（cmd_no=0x02, ack_flag=0x00）写回串口

### 8.4 下行（Web→SQLite→设备）
1. Web 网管改参（写 DB 的 state=1）
2. `sqlite_set_param` 轮询发现变更 → 组装 `Smgmt_set_param` → `mgmt_netlink_set_param` 下发
3. 下发成功后清 state，并 `persist_test_db()`

## 九、工控屏串口部分（你的工作重点：ui_get）
### 9.1 读写线程职责
- 读线程：`get_ui_Thread` → `get_ui_info` → `process_uart_info`
  - 负责把“字节流”转成“命令语义”，并给出 ACK
  - 关键入口：[ui_get.c:process_uart_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L1015-L1148)
- 写线程：`write_ui_Thread`
  - 负责把本机状态“编码成上报帧”，周期推给工控屏
  - 关键入口：[ui_get.c:write_ui_Thread](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L1201-L1269)

### 9.2 命令帧协议（0x01 写参数）
- 帧格式：
  - `D5 5D` + `cmd(01)` + `ack_flag(FF)` + `len` + `addr(4)` + `value(N)` + `crc(2)` + `5D D5`
- CRC：
  - 算法实现：[ui_get.c:CRC_Check](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L659-L681)
  - 校验覆盖范围：从 `cmd` 到 `value`（不含帧头尾）
- 大小端：
  - `addr` 与 4字节 `value` 采用大端（网络序）；解析时将 4字节 value 转主机序（`ntohl`）

### 9.3 地址→业务映射（落地到系统行为）
地址枚举来源：[enum_uartparam_addr.h](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/enum_uartparam_addr.h)

- 执行入口：[ui_get.c:process_cmd_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L774-L969)
- 典型映射（面试建议挑 3~5 个讲清楚“输入→处理→下发→持久化”）：
  - `PARAM_CH1_FIXED_FREQ_CENTER`：频率（kHz）→ 限幅 → 下发 `MGMT_SET_FREQUENCY` → 更新 `systemInfo.rf_freq`
  - `PARAM_CH1_SIGNAL_BANDWIDTH`：带宽枚举 → 下发 `MGMT_SET_BANDWIDTH`
  - `PARAM_CH1_MODULATION_WIDEBAND`：MCS → 下发 `MGMT_SET_UNICAST_MCS` → 更新 `systemInfo.m_rate`
  - `PARAM_CH1_ROUTING_PROTOCOL`：路由协议 → 调脚本切换（olsr/aodv/batman）→ 更新 `/etc/node_xwg`
  - `PARAM_TXRX_INFO_OPERATION`：统计控制 → 改变 `stat_info.stat_flag` 影响上报统计

### 9.4 ACK 逻辑（为什么工控屏需要它）
- 在 `process_uart_info` 完成执行后，按原帧结构回写 ACK：
  - `cmd_no=0x02`、`ack_flag=0x00`、重新计算 CRC
  - 这让工控屏可以做“下发确认”和“失败重试”

### 9.5 周期上报帧（0x04~0x09）
- 由 `write_ui_Thread` 周期发送，常用于工控屏展示与状态刷新：
  - 0x04：模式与通道配置类
  - 0x08：电磁环境/运行参数（时间、MCS、带宽、频点等）
  - 0x05/0x06/0x07/0x09：设备基础信息/统计/自检/邻居拓扑
- 帧构造函数可从 [ui_get.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c) 内的 `Send_0x04/05/06/07/08/09` 追踪

### 9.6 详细流程图：工控屏 0x01 写命令 → 执行 → ACK → DB（重点）
入口代码：[process_uart_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L1015-L1148)、[process_cmd_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L774-L969)

```mermaid
flowchart TD
  S0[工控屏/脚本发送命令帧 0x01\nD5 5D | 01 | FF | Len | Addr | Value | CRC | 5D D5] --> S1[get_ui_info()\nread(/dev/ttymxc2)]
  S1 --> S2{len > 0?}
  S2 -- 否 --> S1
  S2 -- 是 --> S3[process_uart_info(fd, buf, len)]

  S3 --> H0{帧头 == D5 5D?}
  H0 -- 否 --> H1[丢弃/返回]
  H0 -- 是 --> T0{cmd_type == 0x0A?}
  T0 -- 是 --> A0[校验 0x0A 尾/CRC] --> A1[process_cmd_info(PARAM_0A_REQUEST_ADDR, node_id)] --> A2[结束\n(0x0A 不回 0x02 ACK，触发后续成员信息上报)]
  T0 -- 否 --> T1{cmd_type == 0x01?}
  T1 -- 否 --> H1
  T1 -- 是 --> L0[读取 Len 字段\nexpected_total_len = 8 + cmd_len]
  L0 --> L1{len >= expected_total_len?}
  L1 -- 否 --> L2[等待更多/返回\n(短读/粘包时可能触发)]
  L1 -- 是 --> E0{帧尾 == 5D D5?}
  E0 -- 否 --> E1[丢弃/返回]
  E0 -- 是 --> C0[计算 CRC16\nCRC_Check(buf[2 : expected_total_len-4])]
  C0 --> C1{recv_crc == calc_crc?}
  C1 -- 否 --> C2[丢弃/返回]
  C1 -- 是 --> P0[解析 Addr(4字节大端)\n解析 Value(N)]

  P0 --> PL{param_len = cmd_len - 1 - 4\nN==1/2/4?}
  PL -- 1 --> V1[取 u8 值] --> X0
  PL -- 2 --> V2[取 u16 值\nntohs 转主机序] --> X0
  PL -- 4 --> V4[取 u32 值\nntohl 转主机序] --> X0
  PL -- other --> H1

  X0[process_cmd_info(addr, value)]

  X0 --> M0{该参数需要下发到内核/仿真? (isset==TRUE)}
  M0 -- 否 --> DB0{是否仅更新本地状态/文件?}
  DB0 -- 是 --> DB1[可能更新 /etc/node_xwg\n或更新 systemInfo 表] --> ACK0
  DB0 -- 否 --> ACK0

  M0 -- 是 --> N0[组装 Smgmt_set_param\nmhead->mgmt_type |= MGMT_SET_*]
  N0 --> N1[mgmt_netlink_set_param(buffer, buflen, NULL)\n(对接内核/虚拟网卡层；无底层时进入仿真层)]
  N1 --> N2[更新 SQLite（systemInfo/meshInfo）\nupdateData_systeminfo_qk 等]
  N2 --> N3[persist_test_db()\n安全复制到 /www/cgi/test.db\n前端无需改动即可刷新]
  N3 --> ACK0

  ACK0[组装并回写 ACK 帧 0x02\ncmd_no=0x02, ack_flag=0x00\n重新计算 CRC16] --> ACK1[write(fd, ack_frame)]
  ACK1 --> DONE[工控屏收到 ACK\n可做确认/重试]
```

流程图说明（面试提示）：
- 严格校验：帧头/长度/帧尾/CRC4个关口，保证串口噪声或粘包不会误触发下发
- 数据一致性：一次下发会同时影响“内核/仿真运行态 + 本地配置 + SQLite + Web展示”
- ACK闭环：工控屏侧可以把 0x02 作为“下发成功确认”

## 十、Web/数据库同步机制（不改 www 的落地方式）
- DB 文件位置：
  - 源：`/www/cgi-bin/test.db`（校验通过后复制到）
  - 目标：`/www/cgi/test.db`（前端读取）
  - 备份：`/www/cgi/test.db.bak`
  - 临时：`/www/cgi/test.db.tmp`
- 写库流程：
  - 参数更新后调用 `persist_test_db()` 执行“拷贝 + fsync + rename”安全替换（见 [sqlite_unit.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sqlite_unit.c#L1-L200)）
  - WAL、FULL 同步模式，提升抗崩溃能力与一致性

## 十一、仿真层（Simulation）
- 目的：在无真实底层硬件的情况下，保证系统端到端可验证
- 机制：
  - 初始化与心跳：在 [sim_heartbeat.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sim_heartbeat.c) 中同步全局初值（`FREQ_INIT`/`POWER_INIT`/`BW_INIT` 等），确保 Web 与串口上报一致
  - 参数下发路径：`mgmt_netlink_set_param` 被仿真替换/拦截，更新本地状态并驱动 DB 写入与前端刷新

## 十二、测试与验证（面试可演示）
- 串口注入（工控屏模拟）：
  - 使用串口工具或注入脚本发送 0x01 命令帧；板端日志应出现 `[UART DEBUG] Read ... / Recv ...` 并收到 0x02 ACK
  - 周期性帧（0x04~0x09）应能在接收工具看到，表明设备端上报正常
- 网管联通：
  - 观察 `mgmt_get_msg` 周期日志与 DB 表更新，Web 页面数据刷新（不改 www）
  - 多点设置与拓扑转发，通过 `mgmt_recv_msg` 路径验证
- DB 持久化：
  - `persist_test_db()` 不报错；`/www/cgi/test.db` 文件校验通过，前端数据一致

## 十三、简历写法（可直接复制）
### 13.1 简历一句话概述（建议）
- 基于 IMX6ULL/Linux 实现“工控屏串口协议 + 网管消息通道 + SQLite 驱动 Web 展示”的端到端系统，支持参数下发/ACK 确认、周期状态上报与无底层硬件的仿真演示。

### 13.2 你负责的工作（强调 ui_get）
- 设计并实现工控屏串口协议栈：帧解析、CRC16 校验、ACK 回包、参数地址映射与命令执行（[ui_get.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c)）
- 打通参数下发与数据一致性：工控屏/网管下发 → netlink/仿真层执行 → SQLite 更新 → DB 持久化供 Web 刷新
- 构建周期性上报体系：0x04~0x09 状态帧编码与定时上报，工控屏可实时显示设备状态
- 引入仿真心跳：无真实底层时仍可稳定演示整链路，便于测试与面试展示（[sim_heartbeat.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sim_heartbeat.c)）

### 13.3 面试可讲的“问题→方案→效果”
- 工控屏命令需要可靠确认：引入 CRC16 + ACK 回包机制，保证下发可验证与可重试
- Web 前端不允许改动：通过 SQLite 数据桥接与安全持久化复制，实现“后端更新→前端自动刷新”
- 底层硬件不可用：引入仿真层拦截/替换 netlink 返回数据，使系统可在无硬件环境完整运行

## 十四、面试自述稿（2分钟版 / 5分钟版）
### 14.1 2分钟版（快速讲清主线）
1. 我做的是一个基于 IMX6ULL 的端到端通信与展示系统，核心是把工控屏串口、网管消息和 Web 展示串成一条可验证链路。
2. 我负责的重点是工控屏这侧（[ui_get.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c)）：定义并实现了串口帧协议（帧头/尾、Len、Addr、Value、CRC16），并完成 0x01 写命令与 0x02 ACK 的闭环。
3. 下发路径上，工控屏写命令会进入 `process_uart_info` 做完整校验，然后映射成业务参数，组装 `Smgmt_set_param` 通过 `mgmt_netlink_set_param` 下发到系统运行态；同时我会把变更写入 SQLite，并通过 `persist_test_db()` 安全复制到 Web 读取路径，实现“不改前端也能刷新”。
4. 上报路径上，串口写线程周期构造 0x04~0x09 上报帧，让工控屏持续看到设备状态；网管侧线程周期写库，Web 用 DB 展示。
5. 因为底层硬件在部分环境不可用，我做了仿真层，把关键参数（频率/带宽/MCS/功率等）在无硬件时也能更新并驱动 DB，使整链路可测试、可演示。

### 14.2 5分钟版（讲细节与工程点）
1. 背景与约束：
   - 设备侧是 IMX6ULL Linux，前端 `www/` 不能改；底层虽通过 netlink 对接虚拟网卡/内核，但内核细节不作为本项目重点。
   - 目标是让工控屏可以稳定下发参数并得到确认，同时 Web 页面可以稳定展示最新状态。
2. 架构拆分（按职责隔离）：
   - 工控屏串口层：读线程负责命令解析与 ACK，写线程负责周期上报（[write_ui_Thread](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L1201-L1269)）。
   - 中控业务层：`process_cmd_info` 做“地址→业务参数”的映射，并决定是否需要下发与写库（[process_cmd_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L774-L969)）。
   - DB层：SQLite 作为 Web 与中控的桥梁；持久化采用拷贝+fsync+rename，避免前端读到损坏文件（[persist_test_db](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sqlite_unit.c#L1-L200)）。
3. 工控屏下发的落地细节（我负责的重点）：
   - 帧协议：D5 5D 头，5D D5 尾；CRC16（A001 多项式），覆盖范围从 cmd 到 value（[CRC_Check](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L659-L681)）。
   - 解析：`process_uart_info` 做头/长度/尾/CRC 四级校验，解析 addr/value 并统一进入 `process_cmd_info`；执行后回写 0x02 ACK，便于工控屏做重试策略。
   - 地址映射：频率（kHz 大端/ntohl）、带宽枚举、MCS、路由协议切换、工作模式切换等，并且在必要时更新 `/etc/node_xwg`，保持配置可持久化。
4. 数据一致性（为什么 Web 能刷新）：
   - 下发不仅改运行态，还会写 SQLite，并调用 `persist_test_db()` 将 DB 同步到 Web 的读取路径，因此前端不改也能看到变更。
   - 同时，周期上报帧也会体现参数变化，工控屏和 Web 展示保持一致。
5. 可测试性与仿真：
   - 在底层硬件不可用的环境，通过仿真层拦截/替换 netlink 返回数据，使 `mgmt_netlink_set_param` 仍可落地到“运行态 + DB”，保证演示完整链路。
6. 可量化结果（你可以按真实情况补充数字）：
   - 串口协议闭环：支持 N 条关键写命令，均可 ACK 确认并在 Web/工控屏双端同步可见。
   - DB 持久化：避免 DB 部分写入导致的前端异常，提升系统稳定性。

---

## 附：关键代码参考
- 入口与线程创建：[main.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/main.c#L34-L55)
- CRC16 算法与串口解析：[ui_get.c:CRC_Check](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L659-L681)、[ui_get.c:process_uart_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L1015-L1148)
- 命令执行与参数下发：[ui_get.c:process_cmd_info](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c#L774-L969)
- 周期上报帧：`Send_0x04`/`Send_0x08` 等（见 [ui_get.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/ui_get.c)）
- 网管收发与拓扑处理：[mgmt_transmit.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/mgmt_transmit.c)
- SQLite 持久化封装：[sqlite_unit.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sqlite_unit.c)
- 仿真心跳层：[sim_heartbeat.c](file:///home/ssq/linux/IMX6ULL/project/ui_practice_test/sim_heartbeat.c)
