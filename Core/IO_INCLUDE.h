#ifndef IO_INCLUDE
#define IO_INCLUDE
#include <Arduino.h>

#define OS_MAX_TASK 8    //最大可“多进程”执行8个程序
#define OS_MAX_PARAM 16  //子程序传入参数的最大量（16）
#define OS_MAX_SGL_PG 2048
#define OS_MAX_MEM 8192
#endif

typedef enum {
	MOV, // 将参数2（立即数或地址，BYTE,INT,FLOAT）存入参数1表示的地址内存
	EXT_BYTE,//将一个字节类型（变量）扩展为四字节int类型
	SETARRAY,//设置数组元素
	READARRAY,//读取数组某个下标的元素
	INITARRAY,//初始化数组（为数组元素批量赋值）
	PUSH,//将CalcResu的值保存于参数表示的地址中
	PUSHP,//为子程序传入参数
	ADD, // 参数1 + 参数2 → CalcResu
	SUB, // 减法
	MUL, // 乘法
	DIV, // 除法
	EQ, // 判断参数1 == 参数2，结果存CalcResu
	LT, // 参数1 < 参数2
	GT, // 参数1 > 参数2
	LE, // 参数1 <= 参数2
	GE, // 参数1 >= 参数2
	NE, // 参数1 != 参数2
	//使用ARS特制编程语言编写的程序中各项LABEL和FUNC的位置将由编译器进行推定
	JMP, // 无条件跳转到相应的跳转标签
	JMP_T, // 条件跳转（如果CalcResu!=0则跳转）
	CALL,//子程序跳转
	RET,//子程序返回
	BIT_AOX,// 按位与、或
	BIT_MOV,// 按位移
	ARS_TIMER,// 获取系统时钟
	GPIO_WRITE,// 写GPIO
	GPIO_READ,// 读GPIO
	VAL,// 设置临时计算寄存器
	TO_INT,     // 将内存中的浮点数转换为整数存入 CalcResu
	TO_FLOAT,    // 将内存中的整数转换为浮点数存入 CalcResu
	HLT, // 程序结束标记
} Opcode;

volatile void *ARS_memmove(volatile void *dest, volatile const void *src, int n);
volatile void *ARS_memset(volatile void *dest, volatile const void *byte, int n);
uint16_t ARS_strlen(const char *str);
uint8_t ARS_strcmp(const char *haystack, const char *needle, int len);
char *ARS_strtok(char *str, const char delim);
float tranIntToFloat(int x);
int tranFloatToInt(float x);
