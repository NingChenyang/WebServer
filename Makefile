# 编译器设置
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g

# 目标文件
TARGET = server
HTTP_TARGET = server

# 使用wildcard自动获取所有.cpp文件
TCP_SRCS := $(wildcard tcp/*.cpp)
HTTP_SRCS := $(wildcard http/*.cpp)
LOG_SRCS := $(wildcard log/*.cpp)

# 合并所有源文件
ALL_SRCS = HTTP.cpp $(TCP_SRCS) $(HTTP_SRCS) $(LOG_SRCS)

# 头文件
INCLUDES = -I./

# 默认目标
all: $(HTTP_TARGET)

# 编译HTTP服务器
$(HTTP_TARGET): $(ALL_SRCS)
	$(CXX) $^ -o $@ $(CXXFLAGS) -pthread

.PHONY: clean
clean:
	rm -f $(HTTP_TARGET)
clean_log:
	rm -f logs/*.log