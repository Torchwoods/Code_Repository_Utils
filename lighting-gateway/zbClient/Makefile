SBU_REV = "0.1"

GCC = arm-linux-gcc

CFLAGS = -Wall -DVERSION_NUMBER=${SBU_REV}
OBJECTS = zbClient.o socket_client.o
DEFS += -D_GNU_SOURCE 
INCLUDE = -I./

APP_NAME = zbClient.bin 

.PHONY: all,clean
${APP_NAME}:${OBJECTS}
	@$(GCC) $(CFLAGS) $(OBJECTS) -o ${APP_NAME}  -lpthread
	@cp $@ ../
%.o:%.c
	@${GCC} ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $< 
all: ${APP_NAME}
clean:
	@rm -rf *.o ${APP_NAME} ${OBJECTS} ../zbClient.bin

