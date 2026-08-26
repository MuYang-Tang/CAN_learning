/*
 * receive.c - CAN 接收程序 (类似 candump)
 * 用法: ./receive -i vcan0    Ctrl+C 退出
 */

#include "can_common.h"
#include <signal.h>
#include <time.h>
#include <getopt.h>

static volatile sig_atomic_t running = 1;
static unsigned long total_frames = 0;

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[])
{
    const char *ifname = "vcan0";
    int opt;
    int sock;

    while ((opt = getopt(argc, argv, "i:h")) != -1) {
        switch (opt) {
        case 'i': ifname = optarg; break;
        case 'h':
        default:
            fprintf(stderr, "用法: %s -i <iface>\n", argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    sock = open_can_socket(ifname);
    if (sock < 0)
        return 1;

    signal(SIGINT, signal_handler);

    struct can_frame frame;
    ssize_t nbytes;
    time_t start = time(NULL);

    printf("监听 %s ... (Ctrl+C 退出)\n", ifname);

    while (running) {
        nbytes = read(sock, &frame, sizeof(frame));
        if (nbytes < 0) {
            if (errno == EINTR && !running)
                break;
            perror("read");
            break;
        }
        if ((size_t)nbytes != sizeof(frame))
            continue;

        print_frame(ifname, &frame);
        total_frames++;
    }

    close(sock);
    unsigned long el = (unsigned long)(time(NULL) - start);
    printf("\n===== 统计 =====\n");
    printf("总帧数: %lu\n", total_frames);
    if (el > 0)
        printf("运行 %lu 秒, 平均 %.1f 帧/秒\n", el,
               (double)total_frames / el);
    printf("===============\n");

    return 0;
}