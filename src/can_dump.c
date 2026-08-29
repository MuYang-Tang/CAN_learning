/*
 * can_dump.c - 进阶 CAN 监听工具
 * 功能: 接收打印 + ID 过滤 + 统计
 * 用法:
 *   ./can_dump -i vcan0                 监听所有
 *   ./can_dump -i vcan0 -f 0x123        只显示 ID=0x123
 *   ./can_dump -i vcan0 -f 0x100 -m 0x7F0  掩码过滤
 */

#include "can_common.h"
#include <signal.h>
#include <time.h>
#include <getopt.h>

static volatile sig_atomic_t running = 1;
static unsigned long total = 0;
static unsigned long filtered = 0;

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[])
{
    const char *ifname = "vcan0";
    uint32_t filter_id = 0;
    uint32_t filter_mask = 0;   /* 0 = 不过滤 */
    int opt;
    int sock;

    while ((opt = getopt(argc, argv, "i:f:m:h")) != -1) {
        switch (opt) {
        case 'i': ifname = optarg; break;
        case 'f': filter_id = (uint32_t)strtoul(optarg, NULL, 0); break;
        case 'm': filter_mask = (uint32_t)strtoul(optarg, NULL, 0); break;
        case 'h':
        default:
            fprintf(stderr,
                "用法: %s -i <iface> [-f <id>] [-m <mask>]\n"
                "  -f <id>   只显示匹配 ID 的帧\n"
                "  -m <mask> 掩码 (1=必须匹配, 0=忽略)\n"
                "  例: -f 0x100 -m 0x7F0  匹配 0x10x 系列\n", argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    sock = open_can_socket(ifname);
    if (sock < 0)
        return 1;

    /* 设置内核过滤器 */
    if (filter_mask != 0) {
        struct can_filter rfilter[1];
        rfilter[0].can_id   = filter_id;
        rfilter[0].can_mask = filter_mask;
        if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER,
                       &rfilter, sizeof(rfilter)) < 0) {
            perror("setsockopt(CAN_RAW_FILTER)");
            close(sock);
            return 1;
        }
        printf("内核过滤: id=0x%X mask=0x%X (只有匹配帧进入用户态)\n",
               filter_id, filter_mask);
    }

    signal(SIGINT, signal_handler);

    struct can_frame frame;
    ssize_t n;
    time_t start = time(NULL);

    printf("监听 %s ... (Ctrl+C 退出)\n", ifname);

    while (running) {
        n = read(sock, &frame, sizeof(frame));
        if (n < 0) {
            if (errno == EINTR && !running)
                break;
            perror("read");
            break;
        }
        if ((size_t)n != sizeof(frame))
            continue;

        total++;

        /* 用户态过滤 (如果用掩码无法完全表达的需求) */
        if (filter_mask == 0 && filter_id != 0) {
            uint32_t fid = frame.can_id & CAN_SFF_MASK;
            if (fid != filter_id) {
                filtered++;
                continue;
            }
        }

        print_frame(ifname, &frame);
    }

    close(sock);
    unsigned long el = (unsigned long)(time(NULL) - start);
    printf("\n===== 统计 =====\n");
    printf("接收总数: %lu (内核过滤后)\n", total);
    if (filter_mask == 0 && filter_id != 0)
        printf("用户态过滤掉: %lu 帧\n", filtered);
    if (el > 0)
        printf("运行 %lu 秒, 实际显示 %.1f 帧/秒\n", el,
               (double)(total - filtered) / el);
    printf("===============\n");

    printf("===============\n"); // 结束

    if (total == 0) {
        printf("没有接收到任何帧, 请检查接口 %s 是否正确配置\n", ifname);
        printf("可以使用 'ip link show' 查看接口状态\n");
        printf("如果是虚拟 CAN, 请确保已运行 'scripts/setup_vcan.sh'\n");
    }

    if (total == 0) { }

    if (total == 0) {
        printf("没有接收到任何帧, 请检查接口 %s 是否正确配置\n", ifname);
        // printf("可以使用 'ip link show' 查看接口状态\n");
        // printf("如果是虚拟 CAN, 请确保已运行 'scripts/setup_vcan.sh'\n");
    }

    if (total == 1) {
        printf("没有接收到任何帧, 请检查接口 %s 是否正确配置\n", ifname);
        printf("可以使用 'ip link show' 查看接口状态\n");
        printf("如果是虚拟 CAN, 请确保已运行 'scripts/setup_vcan.sh'\n");
    }

    if (total == 0) {
        printf("没有接收到任何帧, 请检查接口 %s 是否正确配置\n", ifname);
    }

    return 0;
}