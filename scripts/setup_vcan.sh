#!/bin/bash
# setup_vcan.sh - 一键创建 vcan 虚拟 CAN 接口
# 用法: ./scripts/setup_vcan.sh [接口名...]  默认创建 vcan0 vcan1
# 每次 WSL 重启后需重新执行

set -e

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info()  { echo -e "${GREEN}[INFO]${NC} $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 检查 root
if [ "$EUID" -ne 0 ]; then
    warn "需要 root 权限，自动切换到 sudo..."
    exec sudo bash "$0" "$@"
fi

# 加载 vcan 模块
if ! modprobe vcan 2>/dev/null; then
    error "无法加载 vcan 内核模块"
    error "请确认是 WSL2 (wsl -l -v)，WSL1 不支持 vcan"
    exit 1
fi
info "vcan 内核模块已加载"

# 默认接口
if [ $# -eq 0 ]; then
    IFACES=("vcan0" "vcan1")
else
    IFACES=("$@")
fi

for iface in "${IFACES[@]}"; do
    if ip link show "$iface" >/dev/null 2>&1; then
        warn "接口 $iface 已存在，重新启动"
        ip link set "$iface" down 2>/dev/null || true
        ip link set "$iface" up
    else
        ip link add dev "$iface" type vcan
        ip link set up "$iface"
    fi
    info "接口 $iface 已就绪"
done

echo ""
info "当前 vcan 接口状态:"
ip link show | grep -E "vcan[0-9]+" || true
echo ""
info "验证 (另开终端): 终端1: candump vcan0  终端2: cansend vcan0 123#DEADBEEF"