//Glue.h是一个用户可以手动扩展的接口
//只需完成胶水代码就可以调用原生ABI
//在计算密集部分，使用原生ABI将会是非常高效的做法
//甚至可以将中断处理移入这个部分，内核部分极快的任务时间片轮转可以有效防止中断响应时间过长的情况
//该说不说很多设计都是我当初的无心之举，后来才发现还有隐藏优势和用途（笑

#ifndef IO_INCLUDE
#include "IO_INCLUDE.h"
#endif

#include <Arduino.h>

#define ABI_QUANTITY 3

typedef void (*ABI)(uars_i8 *, uars_i16);
extern ars_i32 CalcResu[OS_MAX_TASK];  //用于存放调用完外部原生函数后的结果（如果有需要的话）

//ABI调用号
enum ABI_CALL {
	GPIOWRITE = 0,
	GPIOREAD,
	ARSTIMER
};

void gWrite(uars_i8 *, uars_i16);
void gRead(uars_i8 *, uars_i16);
void Timer(uars_i8 *, uars_i16);

ABI ABIs[ABI_QUANTITY] = {
	//指向胶水函数
	//使得ArtisanOS可以通过ABIs这个统一接口来调用外部原生函数
	[GPIOWRITE] = gWrite,
	[GPIOREAD] = gRead,
	[ARSTIMER] = Timer
};

void gWrite(uars_i8 *SerializeParamStack, uars_i16 taskId) {
	int pin = *(int *)SerializeParamStack;
	int val = *(int *)(SerializeParamStack + 4);
	pinMode((uars_i8)pin, OUTPUT);
	digitalWrite((uars_i8)pin, (uars_i8)val);
}

void gRead(uars_i8 *SerializeParamStack, uars_i16 taskId) {
	int pin = *(int *)(SerializeParamStack);
	pinMode((uars_i8)pin, INPUT);
	ars_i32 read = digitalRead(pin);
	ARS_memset(&CalcResu[taskId], &read, 4);
}

void Timer(uars_i8 *SerializeParamStack, uars_i16 taskId) {
	uars_i32 time = millis();
	ARS_memset(&CalcResu[taskId], &time, 4);
}
