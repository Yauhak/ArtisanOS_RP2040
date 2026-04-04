#ifndef IO_INCLUDE
	#include "IO_INCLUDE.h"
#endif

#ifndef INTERPRETER
	#define INTERPRETER
#endif

/*
	该部分为“天工”操作系统的程序执行逻辑部分
	通过该系统独有的字节码虚拟机执行已编译为字节码的程序
	同时该虚拟机可以实现简单的时间片轮转和任务调度
	来实现“操作系统”的要求
*/

//内存读写需要考虑多任务情况
//每个函数应显式要求提供程序ID
//以防止时间片轮转后ID切换，导致读取其他程序的内存
int8_t mov(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t invoke_array(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t push(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t pushp(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t call(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t ret(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t conds(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId);
int8_t jmp(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t jmp_t(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t calc(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId);
int8_t bit_and_or_xor(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t bit_move(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t ars_timer(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t gpio_write(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t gpio_read(uint8_t ParamType, int32_t *params, uint16_t taskId);
int8_t hlt(uint8_t ParamType, int32_t *params, uint16_t taskId);

int8_t interprete(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId);
