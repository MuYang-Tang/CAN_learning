#!/bin/bash

# ============================================
# 一键提交脚本 (Git Bash / WSL 环境使用)
# 用法: ./commit.sh "提交信息"
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

# 暂存所有修改（-A 会包含新增的未跟踪文件；注意：-u 只会暂存已跟踪文件，会漏掉新文件）
git add -A

# 提交
git commit -m "$commit_msg"

# 推送
git push

echo "✅ 提交完成: $commit_msg"
