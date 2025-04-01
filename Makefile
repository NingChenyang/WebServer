# 编译器设置
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g

# 目录设置
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# 版本信息
VERSION = 1.0.0

# 目标文件
TARGET = server
HTTP_TARGET = server
WS_TARGET = WS_Server

# 使用wildcard自动获取所有.cpp文件 
TCP_SRCS := $(wildcard tcp/*.cpp)
HTTP_SRCS := $(wildcard http/*.cpp)
LOG_SRCS := $(wildcard log/*.cpp)
MYSQL_SRCS	:=$(wildcard mysql/*.cpp)
JSON_SRCS :=$(wildcard jsoncpp/jsoncpp.cpp)
WEBSOCKET_SRCS := $(wildcard websocket/*.cpp)

# 合并所有源文件
ALL_SRCS = HTTP.cpp $(TCP_SRCS) $(HTTP_SRCS) $(LOG_SRCS) $(MYSQL_SRCS) $(JSON_SRCS) handle.cpp
WS_SRCS = WS_Server.cpp $(WEBSOCKET_SRCS) $(TCP_SRCS) $(LOG_SRCS) $(HTTP_SRCS)

# 生成对应的.o文件列表
ALL_OBJS = $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp,%.o,$(ALL_SRCS)))
WS_OBJS = $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp,%.o,$(WS_SRCS)))

# 头文件
INCLUDES = -I./

# MySQL配置
MYSQL_CFLAGS = $(shell mysql_config --cflags)
MYSQL_LIBS = $(shell mysql_config --libs)

# 更新CXXFLAGS
CXXFLAGS += $(MYSQL_CFLAGS)

# 默认目标
all: init $(HTTP_TARGET) $(WS_TARGET)

# 初始化目录
init:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)/tcp
	@mkdir -p $(OBJ_DIR)/http  
	@mkdir -p $(OBJ_DIR)/log
	@mkdir -p $(OBJ_DIR)/mysql
	@mkdir -p $(OBJ_DIR)/jsoncpp
	@mkdir -p $(OBJ_DIR)/websocket
	@echo $(VERSION) > $(BUILD_DIR)/VERSION

# 编译HTTP服务器
$(HTTP_TARGET): $(ALL_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) -pthread $(MYSQL_LIBS)

# 编译WebSocket服务器
$(WS_TARGET): $(WS_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) -pthread

# 编译.o文件的规则
$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean init

clean_all:
	rm -rf $(BUILD_DIR)
	rm -f logs/*.log
	rm -f core.*
clean_logs:
	rm -f logs/*.log

clean:
	rm -f $(WS_OBJS) $(WS_TARGET)