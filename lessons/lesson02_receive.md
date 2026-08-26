# 第 2 课：CAN 接收程序逐行讲解

> 对应源码: src/receive.c

## 1. 程序流程

```
解析参数 → socket() → bind() → 阻塞 read() → 打印帧 → 统计 (Ctrl+C)
```

## 2. 核心原理

### 2.1 接收和发送的相同点

接收和发送前半段完全一样：`socket()` + `ioctl(SIOCGIFINDEX)` + `bind()`。
区别只在于最后的数据方向：

- 发送: `write(s, &frame, sizeof(frame))`
- 接收: `read(s, &frame, sizeof(frame))`

### 2.2 阻塞接收

```c
nbytes = read(sock, &frame, sizeof(frame));
```

`read()` 默认是**阻塞模式**，程序会停在 read 这行，直到：
1. 总线上有 CAN 帧到达
2. 有信号中断（如 Ctrl+C）

### 2.3 CAN 广播特性

这是接收最重要的概念：**你发出的帧，自己也能收到（回环）。**

```
终端1: ./send -i vcan0 -id 0x123 -d DEADBEEF
终端2: ./receive -i vcan0   ← 能看到这帧
终端3: ./receive -i vcan0   ← 也能看到！
```

所有绑定 vcan0 的程序都能收到同样的帧，这正是 CAN 总线的广播本质。

### 2.4 时间戳与统计

```c
time_t start = time(NULL);   // 启动时间
total_frames++;              // 每次收到帧就计数
```

退出时打印：
```
===== 统计 =====
总帧数: 42
运行 10 秒, 平均 4.2 帧/秒
===============
```

## 3. 动手练习

### 练习 1: 验证广播

```bash
# 终端1: 接收
./receive -i vcan0

# 终端2: 用 can-utils 发送
cansend vcan0 123#DEADBEEF
cansend vcan0 456#0102
cansend vcan0 789#AABBCCDD
```

### 练习 2: 两只眼睛

```bash
# 终端1 + 终端2 同时接收
./receive -i vcan0   (终端1)
./receive -i vcan0   (终端2)

# 终端3: 发送
./send -i vcan0 -id 0x100 -d 11223344 -p 500 -c 10
```

两个接收端都能收到同样的 10 帧。

### 练习 3: 高负载

```bash
# 发送端: 高速周期发送
./send -i vcan0 -id 0x200 -d 01020304 -p 1 -c 5000

# 接收端: 观察帧率和统计
./receive -i vcan0
```

## 4. 思考题

1. 如果发送端发的太快，接收端 read 不过来会发生什么？
2. 为什么需要检查 nbytes == sizeof(frame)？
3. 如果我们有两个进程同时 read 同一个 socket，会发生什么？

## 5. 下一步

→ 第 3 课：过滤器与多节点通信 (lesson03_filter.md)