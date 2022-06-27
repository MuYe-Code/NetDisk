SRCS:=$(wildcard *.c)
OUTS1:=$(patsubst %.c, %.o, $(SRCS))
LINK:=-lpthread -lcrypt -lmysqlclient
VERSION:=3.2
TARGET:=server
CC:=gcc

.PHONY:all clean rebuild move1 move2

all:$(OUTS1)
	$(CC) $^ -o $(TARGET) $(LINK)

%.o: %.c
	$(CC) -c $< -o $@ $(LINK)

move1:
	cp *.c ../../net-disk/NetDisk_v$(VERSION)/$(TARGET)
	cp *.h ../../net-disk/NetDisk_v$(VERSION)/$(TARGET)
	cp makefile ../../net-disk/NetDisk_v$(VERSION)/$(TARGET)

move2:
	cp *.c ../../../NetDisk_v$(VERSION)/$(TARGET)
	cp *.h ../../../NetDisk_v$(VERSION)/$(TARGET)
	cp makefile ../../../NetDisk_v$(VERSION)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) $(OUTS1)
rebuild: clean all
