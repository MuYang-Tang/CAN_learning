/*
 * send.c - CAN 发送程序
 *
 * 功能:
 *   - 手动发送一帧
 *   - 周期自动发送 (-p 参数)
 *   - 支持标准帧 / 扩展帧 / 远程帧
 *
 * 用法:
 *   ./send -i vcan0 -id 0x123 -d DEADBEEF            发送一帧
 *   ./send -i vcan0 -id 0x123 -d 01020304 -p 500     每 500ms 发送一次
 *   ./send -i vcan0 -id 0x123 -r                     发送远程帧
 *   ./send -i vcan0 -id 0x1ABCDEF -e -d 11 22        发送扩展帧
 *
 * 退出: Ctrl+C 停止周期发送
 */

#include "can_common.h"
#include <getopt.h>
#include <time.h>

static void usage(const char *prog)
{
    fprintf(stderr,
        "用法: %s [选项]\n"
        "  -i <iface>   CAN 接口名 (默认 vcan0)\n"
        "  -id <id>     CAN ID, 支持 0x 前缀 (默认 0x123)\n"
        "  -d <data>    数据 hex 字符串, 如 DEADBEEF (默认空)\n"
        "  -p <ms>      周期发送间隔毫秒 (默认 0=只发一次)\n"
        "  -e           使用扩展帧 (29 位 ID)\n"
        "  -r           发送远程帧 (请求对方发数据)\n"
        "  -c <count>   发送次数 (默认无限, 配合 -p)\n"
        "  -h           显示帮助\n"
        "\n示例:\n"
        "  %s -i vcan0 -id 0x123 -d DEADBEEF\n"
        "  %s -i vcan0 -id 0x123 -d 01020304 -p 500 -c 10\n",
        prog, prog, prog);
}

int main(int argc, char *argv[])
{
    const char *ifname = "vcan0";
    uint32_t can_id = 0x123;
    uint8_t data[8] = {0};
    int data_len = 0;
    int period_ms = 0;      /* 0 = 只发一次 */
    int use_ext = 0;        /* 是否扩展帧 */
    int send_rtr = 0;       /* 是否远程帧 */
    int count = -1;         /* 发送次数, -1 = 无限 */
    int opt;
    int sock;
    struct can_frame frame;
    struct timespec ts, remaining;

    while ((opt = getopt(argc, argv, "i:id:d:p:erc:h")) != -1) {
        switch (opt) {
        case 'i':
            ifname = optarg;
            break;
        case 'id':
            can_id = (uint32_t)strtoul(optarg, NULL, 0);
            break;
        case 'd':
            data_len = hex_to_bytes(optarg, data);
            if (data_len < 0)
                return 1;
            break;
        case 'p':
            period_ms = atoi(optarg);
            break;
        case 'e':
            use_ext = 1;
            break;
        case 'r':
            send_rtr = 1;
            break;
        case 'c':
            count = atoi(optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    /* 打开 CAN 接口 */
    sock = open_can_socket(ifname);
    if (sock < 0)
        return 1;

    /* 检查 ID 是否超出范围 */
    if (use_ext) {
        if (can_id > CAN_EFF_MASK) {
            fprintf(stderr, "扩展帧 ID 超出范围 (最大 0x%X)\n", CAN_EFF_MASK);
            close(sock);
            return 1;
        }
        can_id |= CAN_EFF_FLAG;
    } else {
        if (can_id > CAN_SFF_MASK) {
            fprintf(stderr, "标准帧 ID 超出范围 (最大 0x%X), 用 -e 使用扩展帧\n", CAN_SFF_MASK);
            close(sock);
            return 1;
        }
    }

    if (send_rtr)
        can_id |= CAN_RTR_FLAG;

    /* 构造 CAN 帧 */
    memset(&frame, 0, sizeof(frame));
    frame.can_id  = can_id;
    frame.can_dlc = send_rtr ? 0 : data_len;  /* 远程帧无数据 */
    memcpy(frame.data, data, data_len);

    printf("发送 %s: ID=0x%X DLC=%d 数据:", ifname,
           frame.can_id & (use_ext ? CAN_EFF_MASK : CAN_SFF_MASK),
           frame.can_dlc);
    for (int i = 0; i < frame.can_dlc; i++)
        printf(" %02X", frame.data[i]);
    printf("%s\n", send_rtr ? " [远程帧]" : "");

    ts.tv_sec  = period_ms / 1000;
    ts.tv_nsec = (period_ms % 1000) * 1000000L;

    int sent = 0;
    while (count < 0 || sent < count) {
        if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
            perror("发送失败");
            close(sock);
            return 1;
        }
        sent++;
        printf("[%d] 发送成功\n", sent);

        if (count < 0 || sent < count) {
            /* 周期等待, 处理被信号中断的情况 */
            remaining = ts;
            while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR)
                ;
        }
    }

    close(sock);
    printf("完成, 共发送 %d 帧\n", sent);
    return 0;
}