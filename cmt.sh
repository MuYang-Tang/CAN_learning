#!/bin/bash
set -e  # 任一 git 命令失败立即退出，避免"显示成功但实际没提交"的误导

# ============================================
# 一键提交脚本 (Git Bash / WSL 环境使用)
# 用法: ./cmt.sh "提交信息"
# ============================================

# 检查是否传入了提交信息
if [ -z "$1" ]; then
    echo "❌ 请提供提交信息"
    echo "用法: $0 \"提交信息\""
    exit 1
fi

commit_msg=$1

# 查看有没有修改
if [ -z "$(git status --porcelain)" ]; then
    echo "⚠️ 没有需要提交的修改"
    exit 0
fi

# 暂存所有修改。必须用 -A：-u 只会暂存已跟踪文件，
# 当存在未跟踪的新文件时会直接报 "nothing added to commit"
git add -A

# 提交
git commit -m "$commit_msg"

# 推送
git push

echo "✅ 提交完成: $commit_msg"
