OUT1:=transferMsg.o command.o processPool.o servertest.o
OUT2:=transferMsg.o command.o processPool.o clienttest.o
TARGET1:=server
TARGET2:=client
all: server client

server:$(OUT1)
	gcc $(OUT1) -o $(TARGET1)

client:$(OUT2)
	gcc $(OUT2) -o $(TARGET2)

%.o:%.c
	gcc -c $< -o $@

clean:
	rm $(TARGET1)
	rm $(TARGET2)

rebuild:
	make clean && make
