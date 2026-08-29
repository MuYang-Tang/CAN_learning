# CAN 通信学习项目（SocketCAN + vcan 软件模拟）

> 目标：用 C 语言实现 CAN 的**发送**与**接收**，零硬件成本，在 Windows 上通过 WSL 的虚拟 CAN 总线完成全部学习。

---

## 一、学习路线总览（6 个阶段）

| 阶段 | 内容 | 产出 |
|------|------|------|
| ① 环境搭建 | 安装 WSL / Ubuntu / 工具链 | 能跑通 `candump` + `cansend` |
| ② CAN 协议基础 | 帧格式、仲裁、位填充、错误 | 能看懂数据帧结构 |
| ③ SocketCAN 入门 | socket / bind / send / recv | 第一个收发程序 |
| ④ 进阶功能 | 过滤器、周期发送、双节点通信 | 收发都熟练 |
| ⑤ 实战练习 | 模拟传感器节点 + 主控节点 | 完整通信链路 |
| ⑥ 硬件迁移 | USB-CAN / 开发板 | 理解 SocketCAN 抽象层 |

---

## 二、阶段 ①：环境搭建（Windows 端）

### 1. 安装 WSL（需要管理员权限，装完重启电脑）

打开 **PowerShell（管理员）**，执行：

```powershell
wsl --install
```

重启后，系统会自动安装 Ubuntu。如果没自动装：

```powershell
wsl --install -d Ubuntu
```

### 2. 进入 WSL 并安装工具

```bash
# 更新软件源
sudo apt update && sudo apt upgrade -y

# 安装编译链 + CAN 工具
sudo apt install -y gcc make can-utils build-essential
```

### 3. 加载 vcan 模块并创建虚拟 CAN 接口

```bash
# 加载虚拟 CAN 内核模块
sudo modprobe vcan

# 创建两个虚拟接口（模拟两个 CAN 节点）
sudo ip link add dev vcan0 type vcan
sudo ip link add dev vcan1 type vcan

# 启动接口
sudo ip link set up vcan0
sudo ip link set up vcan1

# 验证
ip link show vcan0
```

> 💡 以上命令会随 WSL 重启而失效。本项目提供了 `scripts/setup_vcan.sh` 一键执行。
> 也可以加入 `~/.bashrc` 实现每次进入自动配置。

### 4. 验证环境

终端 1：

```bash
candump vcan0
```

终端 2：

```bash
cansend vcan0 123#DEADBEEF
```

终端 1 能看到 `vcan0 123 [4] DE AD BE EF` 即环境就绪 🎉
`candump` 和 `cansend` 本身就是用 C 写的 SocketCAN 工具，可以先拿它们熟悉收发。

---

## 三、阶段 ②：CAN 协议基础（30 分钟）

### CAN 2.0B 四种帧

| 帧类型 | 用途 |
|--------|------|
| 数据帧 | 携带数据（最常用，本项目的核心） |
| 远程帧 | 请求对方发送数据（RTR=1，无数据段） |
| 错误帧 | 出错时自动发出，错误检测机制的一部分 |
| 过载帧 | 接收方忙时延迟后续帧 |

### 标准帧（11 位 ID）格式

```
SOF 仲裁场(11位ID + RTR) 控制场(IDE + DLC) 数据场(0~8字节) CRC场 ACK场 EOF
```

### 扩展帧（29 位 ID）

仲裁场为 29 位 ID，用 `CAN_EFF_FLAG` 标记。

### 关键概念

- **DLC**：数据长度，0~8 字节（CAN FD 可达 64，本项目不涉及）
- **仲裁**：多个节点同时发送时，ID 越小优先级越高
- **位填充**：连续 5 个相同位后插入反相位，保证时钟同步
- **错误处理**：错误计数 > 127 进入 Bus-Off 状态

### 学习建议

- 用 `cansend vcan0 123#0102030405060708` 发一个 8 字节帧
- 用 `candump vcan0 -L` 观察带时间戳的输出
- 手写一个标准帧的位图，加深记忆

---

## 四、阶段 ③：SocketCAN 编程入门（核心）

### SocketCAN 为什么用 socket？

CAN 在 Linux 中被抽象成网络设备，用户态通过 `PF_CAN` 协议簇的 socket 访问，这和 TCP/UDP 编程模型一致，好处是：
- 接口统一（vcan / can0 / slcan 都一样）
- 可以用 select / poll / epoll 做多路复用
- 与网络编程知识互通

### 最小发送程序流程

```
socket(PF_CAN, SOCK_RAW, CAN_RAW)   → 创建 CAN 套接字
struct ifreq + ioctl(SIOCGIFINDEX)  → 获取接口索引
bind()                              → 绑定到 vcan0
struct can_frame + send()           → 发送数据
```

### 最小接收程序流程

```
socket(PF_CAN, SOCK_RAW, CAN_RAW)   → 创建 CAN 套接字
bind()                              → 绑定到 vcan0
read() 或 recvfrom()                → 接收数据
```

### 核心数据结构（`linux/can.h`）

```c
struct can_frame {
    canid_t can_id;   /* 32 位 CAN ID（含标志位：EFF/RTR/ERR） */
    __u8    can_dlc;  /* 数据长度 0~8 */
    __u8    __pad;    /* 填充 */
    __u8    __res0;   /* 保留 */
    __u8    __res1;   /* 保留 */
    __u8    data[8] __attribute__((aligned(8)));  /* 数据 */
};
```

### 本项目源码结构

```
can-learning/
├── README.md              ← 本文件（学习规划）
├── Makefile               ← 一键编译
├── scripts/
│   └── setup_vcan.sh      ← 一键创建虚拟 CAN 接口
├── src/
│   ├── can_common.h       ← 通用头文件（结构体、工具函数）
│   ├── send.c             ← 发送程序（手动输入或自动周期发送）
│   ├── receive.c          ← 接收程序（实时打印收到的帧）
│   └── can_dump.c         ← 类似 candump 的进阶版（带过滤、统计）
└── lessons/
    ├── lesson01_send.md   ← 第 1 课：发送程序逐行讲解
    ├── lesson02_receive.md← 第 2 课：接收程序逐行讲解
    └── lesson03_filter.md ← 第 3 课：过滤器与多节点通信
```

---

## 五、阶段 ④：进阶功能

### 1. 过滤器（setsockopt）

```c
struct can_filter rfilter[1];
rfilter[0].can_id   = 0x123;
rfilter[0].can_mask = CAN_SFF_MASK;  // 只接收 ID=0x123 的帧
setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
```

### 2. 周期发送

`send.c` 中实现了 `-p <毫秒>` 参数，用 `nanosleep` 做周期定时。

### 3. 双节点通信

```
终端1: ./receive             ← 监听 vcan0
终端2: ./send -i vcan0 -id 0x123 -d 01020304
```

再进阶：用 vcan1 + `ip link set vcan1 master vcan0`（vcan 支持跨接口桥接学习用）。

### 4. 多路复用

接收端用 `select()` 同时监听 vcan0 和 stdin，实现命令行控制。

---

## 六、阶段 ⑤：实战练习题目

1. **传感器节点**：每秒发送一次 ID=0x100，携带温度（模拟值）+ 校验和
2. **主控节点**：接收 0x100，校验和正确则回复 0x200 确认帧
3. **错误注入**：手动构造错误帧，观察 error 计数如何变化
4. **多 ID 数据**：发送 4 个不同 ID 的周期数据，接收端按 ID 分类打印
5. **超时检测**：如果 3 秒没收到传感器数据，打印告警

---

## 七、阶段 ⑥：迁移到真实硬件

### SocketCAN 的价值体现

学习代码几乎**零改动**迁移：
- vcan0 → can0 即可（真实 CAN 接口名）

### 硬件方案选择

| 方案 | 成本 | 适合场景 |
|------|------|----------|
| USB-CAN 分析仪（周立功 / 创芯） | 100~300 元 | Windows + 真机调试最方便 |
| STM32 开发板 + TJA1050 | 30~80 元 | 嵌入式方向，深入单片机 CAN 外设 |
| PCAN / Kvaser | 千元以上 | 专业开发/汽车行业 |

### 真实硬件 vs 虚拟区别

| 项目 | vcan | 真实 CAN |
|------|------|----------|
| 波特率 | 无（即时） | 需配置（如 500kbps） |
| 终端电阻 | 无 | 总线两端各 120Ω |
| 物理层 | 无 | 差分电压 H/L |
| 错误帧 | 模拟 | 真实错误检测 |

### WSL 注意⚠️

WSL 默认不暴露 USB 设备。如果要用硬件：
- 方案 A：在 Windows 原生跑（用 USB-CAN 厂商 SDK）
- 方案 B：WSL2 + usbipd-win 转发 USB 设备（较复杂）
- 方案 C：装 Ubuntu 双系统 / 虚拟机直通

---

## 八、日常使用速查

```bash
# 创建虚拟接口（每次重启 WSL 后执行）
./scripts/setup_vcan.sh

# 编译所有程序
make

# 查看 CAN 总线上的所有数据
candump vcan0

# 发送一帧
cansend vcan0 123#DEADBEEF

# 运行自己的发送程序（周期发，2 字节）
./send -i vcan0 -id 0x123 -d DEADBEEF -p 500

# 运行自己的接收程序
./receive -i vcan0

# 进阶接收（带过滤 + 统计）
./can_dump -i vcan0 -f 0x123
```

---

## 九、推荐学习资源

- **Linux 内核文档**：`linux-can` 子系统源码 `net/can/`（学透 SocketCAN 的最终答案）
- **《CAN 总线轻松入门与实践》**：中文入门好书
- **博世 CAN 2.0 协议规范 PDF**：权威原版协议
- **can-utils 源码**：`git clone https://github.com/linux-can/can-utils`（参考 candump/cansend 的实现）

---

## 十、下一步行动清单

- [ ] 用管理员 PowerShell 执行 `wsl --install`，重启电脑
- [ ] 打开 WSL / Ubuntu 终端，安装 gcc/make/can-utils
- [ ] 执行 `scripts/setup_vcan.sh` 创建虚拟接口
- [ ] 用 `candump` + `cansend` 验证环境
- [ ] 阅读 `lessons/lesson01_send.md`，编译运行 `send`
- [ ] 阅读 `lessons/lesson02_receive.md`，编译运行 `receive`
- [ ] 双终端联调，实现“能发能收”
- [ ] 完成实战练习后，再考虑硬件迁移