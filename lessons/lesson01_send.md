# 第 1 课：CAN 发送程序逐行讲解

> 对应源码: src/send.c

## 1. 程序流程

```
解析参数 → socket() → ioctl(SIOCGIFINDEX) → bind() → 构造帧 → write() → 周期循环
```

## 2. 核心步骤

### 2.1 创建套接字

```c
s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
```

| 参数 | 值 | 含义 |
|------|-----|------|
| domain | PF_CAN | CAN 协议族 |
| type | SOCK_RAW | 原始套接字 |
| protocol | CAN_RAW | CAN 原始协议 |

### 2.2 获取接口索引

```c
struct ifreq ifr;
strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
ioctl(s, SIOCGIFINDEX, &ifr);
```

### 2.3 绑定接口

```c
struct sockaddr_can addr;
addr.can_family = AF_CAN;
addr.can_ifindex = ifindex;
bind(s, (struct sockaddr *)&addr, sizeof(addr));
```

### 2.4 构造 CAN 帧

```c
struct can_frame frame;
frame.can_id = 0x123;
frame.can_dlc = 4;
```

### 2.5 发送

```c
write(s, &frame, sizeof(frame));
```

> CAN 是广播协议，没有目标地址。ID 是优先级，不是地址。

## 3. 动手练习

```bash
# 终端1: 监听
./receive -i vcan0

# 终端2: 发送一次
./send -i vcan0 -id 0x123 -d DEADBEEF

# 终端2: 周期发送
./send -i vcan0 -id 0x321 -d 0102030405060708 -p 100 -c 20

# 终端2: 远程帧
./send -i vcan0 -id 0x123 -r

# 终端2: 扩展帧
./send -i vcan0 -id 0x1ABCDEF -e -d 11223344
```

## 4. 思考题

1. 为什么 CAN 不需要对方的地址？
2. ID 越小优先级越高还是越低？
3. 远程帧为什么没有数据段？

## 5. 下一步

→ 第 2 课：接收程序 (lesson02_receive.md)