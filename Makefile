# 编译选项
CC = g++

# 调试选项
CFLAGS = -Wall -g -fPIC
# 生成选项
#CFLAGS = -Wall -O2 -fPIC  

# include path
INC = -I"./src" 

# 宏定义
DEFS = -D_LINUX_
CFLAGS += $(DEFS)

# 添加库
LIBFLAGS = -L ./src/mmo_demo -lpthread -lrt

# 动态库编译
# LDFLAGS := -shared 

# .cpp 源文件定义
SRCS := $(shell ls ./src/*.cpp)

# .o 编译文件定义
OBJS := $(SRCS:.cpp=.o)

# 目标定义
TARGET := ./bin/demo

# 目标生成
$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

# .o 生成
.cpp.o :
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

# 清除：
.PHONY : clean 
clean : 
	rm -f $(TARGET) $(OBJS)	

