# ============================================================
#  单机版小红书 · 后端构建脚本
#
#  Windows 上 make 会根据环境自动挑选执行 recipe 的 shell：
#  Git Bash / MSYS2 里是真 sh，PowerShell / cmd 里退回 cmd.exe。
#  本文件两种都能跑，入口不变：
#
#    make            编译 server.exe
#    make run        编译并启动（默认 8787）
#    make rebuild    清理后全量重编
#    make clean      删除中间产物
#    make help       查看全部目标
#
#  构建输出故意用 [CC] / [LD] 这类 ASCII 标记 —— cmd 与 PowerShell
#  的控制台默认是 GBK 代码页，打印中文会变乱码（注释不受影响）。
#
#  sqlite3.c 有 25 万行，首次编译约 1 分钟，之后靠时间戳增量跳过，
#  别随手 make clean。
# ============================================================

.DEFAULT_GOAL := all

# ---------------- 平台 ----------------
# 三个坑，别改回去：
#  1. 不能写死 SHELL := /bin/sh。PowerShell 里没有 sh.exe，make 会在
#     每条 recipe 上失败并打印「系统找不到指定的路径」。
#  2. Windows 上不能跑 $(shell uname -s)：解析阶段就会失败，哪怕这次
#     根本没有需要编译的目标（表现就是一堆报错 + Nothing to be done）。
#  3. 所以先认 $(OS)，只在非 Windows 上才去问 uname。
ifeq ($(OS),Windows_NT)
  PLATFORM := windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    PLATFORM := darwin
  else
    PLATFORM := unix
  endif
endif

# 当前 recipe 是否真的由 sh 执行：Git Bash 下 $(SHELL) 是绝对路径
# （.../usr/bin/sh.exe），PowerShell 下只是个找不到的 "sh.exe"。
ifneq (,$(findstring /,$(SHELL)))
  POSIX_SHELL := 1
endif

# ---------------- 工具链 ----------------
# make 内置了 CC=cc / CXX=g++，只在用户没显式指定时替换成 MinGW 的
ifeq ($(origin CC),default)
  CC := gcc
endif
ifeq ($(origin CXX),default)
  CXX := g++
endif
PY ?= python

# ---------------- 编译选项 ----------------
OPT  ?= -O2
WARN ?= -Wall -Wextra
STD  ?= -std=c++17

CXXFLAGS += $(STD) $(OPT) $(WARN)
CFLAGS   += $(OPT)

# 自动生成头文件依赖，改了 sqlite3.h 也能触发重编
DEPFLAGS := -MMD -MP

ifeq ($(PLATFORM),windows)
  EXE    := .exe
  LDLIBS += -lws2_32
else
  EXE    :=
  LDLIBS += -lpthread -ldl
endif

# 清理命令要按「recipe 实际由哪个 shell 执行」来选，不能按平台选：
#  - Git Bash 下平台是 Windows，但 recipe 由 sh 跑，用不了 cmd 的 del；
#    而且 MSYS 会把 cmd 的 /C 当成路径改写，`cmd /C ...` 会掉进交互模式。
#  - PowerShell / cmd 下 recipe 由 cmd.exe 跑，没有 rm。
ifeq ($(POSIX_SHELL),1)
  DEL_FILES = rm -f $(1)
  DEL_DIRS  = rm -rf $(1)
  ANDROID_OUT := android-app/build android-app/app/build
else
  # 末尾 exit /b 0 吞掉「文件/目录不存在」的退出码，否则 make 会额外
  # 打印一行 "Error 2 (ignored)"，看着像出错了。
  DEL_FILES = cmd /C "del /Q /F $(1) 2>nul & exit /b 0"
  DEL_DIRS  = cmd /C "rmdir /S /Q $(1) 2>nul & exit /b 0"
  ANDROID_OUT := android-app\build android-app\app\build
endif

# 想要不依赖 MinGW 运行库的单文件 exe：make STATIC=1
ifeq ($(STATIC),1)
  LDFLAGS += -static
endif

# ---------------- 目标文件 ----------------
TARGET  := server$(EXE)
OBJS    := server.o sqlite3.o
DEPS    := $(OBJS:.o=.d)
ANDROID_DIR := android-app

# sh 里跑当前目录的程序要加 ./，cmd 里不能加
ifeq ($(POSIX_SHELL),1)
  RUNNER := ./$(TARGET)
else
  RUNNER := $(TARGET)
endif

# 运行参数（可覆盖：make run PORT=9000）
PORT    ?= 8787
DATADIR ?= data

.PHONY: all run clean rebuild check icon apk help

# ============================================================
#  默认目标
# ============================================================
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

server.o: server.cpp sqlite3.h
	@echo [CC] $<
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

sqlite3.o: sqlite3.c sqlite3.h
	@echo [CC] $< -- first build takes about a minute
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

# ============================================================
#  运行 / 维护
# ============================================================
run: $(TARGET)
	@echo [RUN] http://localhost:$(PORT) -- Ctrl+C to stop
	@$(RUNNER) $(PORT) $(DATADIR)

clean:
	@echo [CLEAN] objects and binary
	-@$(call DEL_FILES,$(OBJS) $(DEPS) $(TARGET))
	-@$(call DEL_DIRS,$(word 1,$(ANDROID_OUT)))
	-@$(call DEL_DIRS,$(word 2,$(ANDROID_OUT)))

rebuild: clean all

check:
	@echo platform   = $(PLATFORM)
	@echo shell      = $(SHELL)
	@echo posix-sh   = $(if $(POSIX_SHELL),yes,no)
	@echo target     = $(TARGET)
	@echo C compiler = $(CC)
	@echo C++ comp.  = $(CXX)

# 生成桌面图标 tools/app.ico（图案与网页 favicon 一致）
icon:
	@echo [ICON] tools/app.ico
	@$(PY) tools/make_icon.py

# 打包安卓 App（需要 JDK 17 + Gradle + Android SDK 35）
apk:
	@echo [APK] assembleRelease
	@cd $(ANDROID_DIR) && gradle assembleRelease

help:
	@echo make            build server$(EXE)
	@echo make run        build and start on port $(PORT)
	@echo make rebuild    clean then full rebuild
	@echo make clean      remove objects, deps, binary, android build dirs
	@echo make check      show toolchain and shell info
	@echo make icon       regenerate tools/app.ico
	@echo make apk        build android release apk
	@echo vars: OPT="-O0 -g"  STATIC=1  PORT=8787  DATADIR=data
