# 编译器设置
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g

# 目标文件
TARGET = server

# 源文件(排除client.cpp)
SRCS = $(wildcard *.cpp)
SRCS := $(filter-out bin/x64/Debug/client.cpp, $(SRCS))

# 头文件
INCLUDES = -I./

# 默认目标
all: $(TARGET)

# 直接编译服务器
$(TARGET): $(SRCS)
	$(CXX) $^ -o $@ $(CXXFLAGS) -pthread

.PHONY: clean
clean:
	rm -f $(TARGET)
