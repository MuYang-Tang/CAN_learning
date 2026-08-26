#ifndef CAN_COMMON_H
#define CAN_COMMON_H

/*
 * can_common.h - SocketCAN 通用头文件
 * 提供: 打开接口、打印帧、hex 转换
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

/*
 * open_can_socket - 打开并绑定 CAN 接口
 * @ifname: 如 "vcan0"
 * @return: socket fd 或 -1
 */
static int open_can_socket(const char *ifname)
{
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("socket");
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl(SIOCGIFINDEX)");
        fprintf(stderr, "接口 %s 不存在? 先运行 scripts/setup_vcan.sh\n", ifname);
        close(s);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    return s;
}

/*
 * print_frame - 打印一帧 CAN 数据 (类似 candump 格式)
 */
static void print_frame(const char *ifname, const struct can_frame *f)
{
    int i;
    char ascii[9];
    int n = 0;

    printf("%-6s ", ifname ? ifname : "can?");

    if (f->can_id & CAN_EFF_FLAG)
        printf("%08X  ", f->can_id & CAN_EFF_MASK);
    else
        printf("%03X    ", f->can_id & CAN_SFF_MASK);

    printf("[%d]  ", f->can_dlc);

    for (i = 0; i < f->can_dlc; i++) {
        printf("%02X ", f->data[i]);
        ascii[n++] = (f->data[i] >= 0x20 && f->data[i] <= 0x7E) ? f->data[i] : '.';
    }
    for (i = f->can_dlc; i < 8; i++)
        printf("   ");

    if (f->can_dlc > 0) {
        ascii[n] = '\0';
        printf(" '%s'", ascii);
    }
    if (f->can_id & CAN_RTR_FLAG)
        printf(" [RTR]");

    printf("\n");
    fflush(stdout);
}

/*
 * hex_to_bytes - hex 字符串转字节数组
 * 支持: "DEADBEEF" / "DE AD BE EF" / "0x12AB"
 * @return: 字节数(0~8) 或 -1
 */
static int hex_to_bytes(const char *hex, uint8_t *out)
{
    int count = 0;
    const char *p = hex;
    int high = 1;
    uint8_t b = 0;

    while (*p) {
        int v;

        if (*p == ' ' || *p == '\t') { p++; continue; }
        if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) { p += 2; continue; }

        if (*p >= '0' && *p <= '9')      v = *p - '0';
        else if (*p >= 'A' && *p <= 'F') v = *p - 'A' + 10;
        else if (*p >= 'a' && *p <= 'f') v = *p - 'a' + 10;
        else {
            fprintf(stderr, "非法 hex 字符: '%c'\n", *p);
            return -1;
        }

        if (high) {
            b = (uint8_t)(v << 4);
            high = 0;
        } else {
            b |= (uint8_t)v;
            if (count >= 8) {
                fprintf(stderr, "超过 8 字节\n");
                return -1;
            }
            out[count++] = b;
            b = 0;
            high = 1;
        }
        p++;
    }

    if (!high) {
        fprintf(stderr, "hex 长度必须为偶数\n");
        return -1;
    }
    return count;
}

#endif /* CAN_COMMON_H */