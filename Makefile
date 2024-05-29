CC = gcc
RM = rm -rf
TARGET = mini_os
CFLAG = -fcommon -w -D _GNU_SOURCE
SRC_DIR = src
OBJ_DIR = obj
INCS = header

SRCS = cat.c cd.c chmod.c cp.c directory.c grep.c group.c ls.c main.c mkdir.c mv.c permission.c queue.c touch.c user.c useradd.c utility.c
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAG) -o $@ $^ -o $@ -I$(INCS) -lpthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAG) -c $< -o $@ -I$(INCS)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(TARGET)

re: fclean
	make all

.PHONY: all clean fclean re
