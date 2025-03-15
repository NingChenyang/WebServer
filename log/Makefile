# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -O2
LDFLAGS = -pthread

# 目标文件
TARGET = logtest_new

# 源文件目录
SRC_DIR = .
PARENT_DIR = ..

# 源文件
SRCS = Logger.cpp \
       LogStream.cpp \
       AsyncLogger.cpp \
       LogFile.cpp \
       FileUtil.cpp \
       ../TimeStamp.cpp \
       ../CurrentThread.cpp \
       ../util.cpp \
       logtest_new.cpp

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 头文件路径
INCLUDES = -I../ -I.

# 默认目标
all: $(TARGET)

# 目标规则
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

# 编译规则
%.o: %.cpp
    $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理规则
.PHONY: clean
clean:
    rm -f $(TARGET) *.o ../*.o *.log core

# 运行规则
.PHONY: run
run: $(TARGET)
    ./$(TARGET)

# 内存检查
.PHONY: memcheck
memcheck: $(TARGET)
    valgrind --tool=memcheck --leak-check=full ./$(TARGET)

# 帮助信息
.PHONY: help
help:
    @echo "可用的 make 目标："
    @echo "make        - 构建测试程序"
    @echo "make clean  - 清理构建文件"
    @echo "make run    - 运行日志测试"
    @echo "make memcheck - 运行内存检查"
    @echo "make help   - 显示此帮助信息"

.PHONY: all clean run memcheck help