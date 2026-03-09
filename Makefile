# 1. 编译器设置
CC = arm-linux-gnueabihf-gcc
# CC = /home/ssq/linux/IMX6ULL/tool/buildroot-2025.02.10/output/host/bin/arm-linux-gnueabihf-gcc

# ================= 新增：libnl 库路径配置 =================
# 请将此路径替换为你刚才编译安装 libnl-3.5.0 时的 install_out 绝对路径
LIBNL_DIR = /home/ssq/linux/IMX6ULL/project/linux_net/libnl-3.5.0/install_out
# ==========================================================

# 2. 编译选项 (CFLAGS)
# -Wall: 打印所有警告   -O2: 代码优化   -g: 包含调试信息(方便日后用gdb调试)
# [新增]: -I$(LIBNL_DIR)/include/libnl3 告诉编译器去哪里找 <netlink/genl/genl.h>
CFLAGS =-std=gnu99 -O2 -g -I$(LIBNL_DIR)/include/libnl3

# 3. 链接选项 (LDFLAGS)
# [新增]: -L$(LIBNL_DIR)/lib 告诉链接器库文件在哪
# [新增]: -lnl-genl-3 -lnl-3 是 libnl 具体需要链接的库 (顺序不能反)
LDFLAGS = -L$(LIBNL_DIR)/lib -lpthread -lm -ldl -lnl-genl-3 -lnl-3

# 4. 目标文件和源文件定义
TARGET = ui_practice

SRCS = main.c \
       ui_get.c \
       socketUDP.c \
       sqlite3.c \
       gpsget.c \
       Thread.c \
       sqlite_unit.c \
       Lock.c \
       SocketTCP.c \
       mgmt_transmit.c \
       mgmt_netlink.c \
       sim_heartbeat.c

# 自动把 .c 替换成 .o
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# 链接生成可执行文件
# 链接时，带有 -l 的 LDFLAGS 放在最后面，保证函数符号能被正确解析
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译 C 文件生成 .o 目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理工程：敲击 make clean 时执行
clean:
	rm -f $(OBJS) $(TARGET)

# 声明伪目标，防止和同名文件冲突
.PHONY: all clean