CC=arm-linux-gnueabihf-gcc
SRC_FILES+=test_tunnel.c
LIBS+=-lpthread
SRC_OBJS+=$(patsubst %.c,%.o, $(SRC_FILES))

all:test_tunnel
	#for i in $(CPP_SRC_TARGETS); do $(CPP) $$i.c -o $$i; done
%.o:%.c
	$(CC) $(CPPFLAGS) -c $< -o $@
test_tunnel:$(SRC_OBJS)
	$(CC) $(SRC_OBJS) $(LIBS) -o $@
clean:
	rm -rf $(SRC_OBJS) test_tunnel
	