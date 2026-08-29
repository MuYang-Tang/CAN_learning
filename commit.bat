@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

REM ============================================
REM 一键提交脚本 (Windows CMD 环境使用)
REM 用法: commit.bat "提交信息"
REM ============================================

REM 检查是否传入了提交信息
if "%~1"=="" (
    echo [ERROR] 请提供提交信息
    echo 用法: commit.bat "提交信息"
    exit /b 1
)

set "commit_msg=%~1"

REM 检查是否在 Git 仓库中
git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 当前目录不是 Git 仓库
    exit /b 1
)

REM 查看有没有修改
set "CHANGED="
for /f "delims=" %%i in ('git status --porcelain') do set "CHANGED=1"
if not defined CHANGED (
    echo [WARN] 没有需要提交的修改
    exit /b 0
)

REM 暂存所有修改（-A 会包含新增的未跟踪文件；若只想暂存已跟踪文件改为 -u）
git add -A
if errorlevel 1 (
    echo [ERROR] git add 失败
    exit /b 1
)

REM 提交
git commit -m "%commit_msg%"
if errorlevel 1 (
    echo [ERROR] git commit 失败
    exit /b 1
)

REM 推送
git push
if errorlevel 1 (
    echo [ERROR] git push 失败
    exit /b 1
)

echo [OK] 提交完成: %commit_msg%
endlocal
