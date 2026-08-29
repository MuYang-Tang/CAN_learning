# CAN 学习项目 Makefile
# 用法:
#   make          - 编译所有程序
#   make clean    - 清理编译产物
#   make install  - 安装到系统路径（可选）

CC      = gcc
CFLAGS  = -Wall -Wextra -O2
TARGETS = send receive can_dump

SRC_DIR = src
BIN_DIR = bin

all: $(TARGETS)

# 静态规则：每个程序对应 src 下的同名 .c 文件
send: $(SRC_DIR)/send.c $(SRC_DIR)/can_common.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/send $(SRC_DIR)/send.c

receive: $(SRC_DIR)/receive.c $(SRC_DIR)/can_common.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/receive $(SRC_DIR)/receive.c

can_dump: $(SRC_DIR)/can_dump.c $(SRC_DIR)/can_common.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/can_dump $(SRC_DIR)/can_dump.c

clean:
	rm -rf $(BIN_DIR)

install: all
	install -m 755 $(BIN_DIR)/send $(BIN_DIR)/receive $(BIN_DIR)/can_dump /usr/local/bin/

.PHONY: all clean install