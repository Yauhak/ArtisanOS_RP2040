#ifndef COMPILER
#define COMPILER
#include <stdint.h>
#define MAX_SYMBOLS 512
#define MAX_LABELS 256
#define MAX_FUNCS 256
#define MAX_TOKEN_LEN 80

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

// 符号表条目
typedef struct {
	char name[32];
	int addr;
} Symbol;

// 标签信息
typedef struct {
	char name[32];
	uint32_t addr;
} Label;

// 子程序信息
typedef struct {
	char name[32];
	uint32_t addr;
} Func;

typedef struct {
	char name[32];
	uint32_t code_addr;
	char is_taken;
} UnfilledLab;

// 编译器全局状态
typedef struct {
	Func functions[MAX_FUNCS];
	Label labels[MAX_LABELS];
	UnfilledLab unlables[MAX_LABELS];
	int func_count;
	int label_count;
	int unlabel_count;
} CompilerState;

#endif
