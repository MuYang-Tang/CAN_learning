#!/bin/bash

# ============================================
# 拉取仓库并创建新分支
# 用法: ./clone.sh <仓库地址> <基础分支> <新分支名> [目标目录] [--single-branch]
# ============================================

# 参数检查
if [ $# -lt 3 ]; then
    echo "❌ 参数不足"
    echo "用法: $0 <仓库地址> <基础分支> <新分支名> [目标目录] [--single-branch]"
    echo ""
    echo "示例:"
    echo "  $0 https://xxx.git develop feature/my-branch"
    echo "  $0 https://xxx.git develop feature/my-branch my-project --single-branch"
    exit 1
fi

repo_url=$1
base_branch=$2
new_branch=$3
shift 3

# 解析可选参数
single_branch=false
target_dir=""
for arg in "$@"; do
    if [ "$arg" = "--single-branch" ]; then
        single_branch=true
    else
        target_dir=$arg
    fi
done

echo "===== 步骤1: 克隆仓库 ====="
echo "仓库地址: $repo_url"
echo "基础分支: $base_branch"

# 构建克隆命令
clone_cmd="git clone -b \"$base_branch\" --recursive"
if [ "$single_branch" = true ]; then
    clone_cmd="$clone_cmd --single-branch"
fi

if [ -n "$target_dir" ]; then
    # 指定了目录：在当前目录下创建子目录
    clone_cmd="$clone_cmd \"$repo_url\" \"$target_dir\""
else
    # 没指定目录：直接克隆到当前目录（用 . 表示）
    clone_cmd="$clone_cmd \"$repo_url\" ."
fi

echo "执行: $clone_cmd"
eval $clone_cmd

if [ $? -ne 0 ]; then
    echo "❌ 克隆失败"
    exit 1
fi

# 进入项目目录
if [ -n "$target_dir" ]; then
    cd "$target_dir"
fi
# 没指定目录时不需要 cd，因为已经克隆到当前目录了

echo ""
echo "===== 步骤2: 创建新分支 ====="
git checkout -b "$new_branch" "origin/$base_branch"

if [ $? -ne 0 ]; then
    echo "❌ 创建分支失败"
    exit 1
fi

echo ""
echo "===== 步骤3: 推送新分支到远程 ====="
git push -u origin HEAD

if [ $? -ne 0 ]; then
    echo "❌ 推送失败"
    exit 1
fi

echo ""
echo "✅ 完成！"
echo "当前分支: $new_branch"
echo "已自动追踪远程分支，后续直接 git push 即可"