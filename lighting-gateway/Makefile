SBU_REV = "0.1"

GCC = arm-linux-gcc
CFLAGS = -Wall -DVERSION_NUMBER=${SBU_REV} 
OBJECTS = zbFwStart.o  zbSocCmd.o zbSocSocketServer.o
DEFS += -D_GNU_SOURCE 
INCLUDE = -I$(PWD)/include/ -I$(PWD)/zbSocDriver/ -I$(PWD)/zbSocSocketServer/ 

APP_NAME = zllGateWay.bin 

.PHONY: all,clean
${APP_NAME}:${OBJECTS}
	@$(GCC) $(CFLAGS) $(INCLUDE) $(OBJECTS) -o ${APP_NAME}  -lpthread
	@make -C ./zbClient/ 
	cp $@ ~/zedboard/filesystem/ 
%.o:./zbFwStart/%.c
	@${GCC} ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $< 
%.o:./zbSocDriver/%.c
	@${GCC} ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $<
%.o:./zbSocSocketServer/%.c
	@${GCC} ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $<
all: ${APP_NAME}
clean:
	@rm -rf *.o ${APP_NAME} ${OBJECTS}
	@make clean -C ./zbClient

