#ifndef IO_INCLUDE
	#include "IO_INCLUDE.h"
#endif

#ifndef INTERPRETER
	#define INTERPRETER
#endif

//内存读写需要考虑多任务情况
//每个函数应显式要求提供程序ID
//以防止时间片轮转后ID切换，导致读取其他程序的内存
ars_i8 mov(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 init_array(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 set_read_array(uars_i8 cmdAndPmTp, ars_i32 *params, uars_i16 taskId);
ars_i8 push(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 pushp(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 call(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 ret(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 conds(uars_i8 cmdAndPmTp, ars_i32 *params, uars_i16 taskId);
ars_i8 jmp(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 jmp_t(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 calc(uars_i8 cmdAndPmTp, ars_i32 *params, uars_i16 taskId);
ars_i8 bit_and_or_xor(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 bit_move(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 abi_invoke(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 reg_write(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 reg_read(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 val(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 to_int(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 to_float(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 hlt(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 ipc_send(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);
ars_i8 ipc_recv(uars_i8 ParamType, ars_i32 *params, uars_i16 taskId);

ars_i8 interprete(uars_i8 cmdAndPmTp, ars_i32 *params, uars_i16 taskId);
