#ifndef IO_INCLUDE
	#include "IO_INCLUDE.h"
#endif

#ifndef MEMORY
	#define MEMORY

	#define OUT_BOUND 1
	#define ID_ERR 2
	#define HEAD_ERR 3
	#define BAD_MEM_TRACE 4
	#define DIV_BY_0 5
	#define OUT_PARAM_BOUND 6
	#define INVALID_LEN 7
	#define NEED_APPEND_TO_TAIL 8
	#define MEM_CLEAN_PARTLY 9
	#define NO_FREE_MEM 10
	#define NO_MEM_TAIL 11
	#define NO_MEM_HEAD 12
	#define BAD_FREE_BLOCK 13


	#define SPLIT "SPLT" //魔术字：已被程序占位
	#define FREE "FREE" //魔术字：程序内存已被清空
	#define CHECK 1145141919
#endif

struct Magic {
	char MagicHead[4];
	uint8_t id;
	int32_t Check;
	//Check为守卫标识
	//最后的内存防线
	//若连此值都被破坏则认为该段内存完全损坏
	uint32_t len;
	//len=数据块长度+4字节上文指针
	//不包括魔术字头
	volatile uint8_t *last_block;
	volatile uint8_t *next_block;
	//作用域的规划将交给ARS字节码编译器处理
	//int level;
	//uint8_t padding[3];
} __attribute__((packed));

typedef struct Magic Magic;


struct ParamStack {
	int8_t Type;
	union {
		int8_t BYTE;
		//int16_t DWORD;现在抛弃了DWORD类型，因为几乎没用
		int32_t INT;
		float FLOAT;
	} DATA __attribute__((aligned(4))); // 4字节对齐
}__attribute__((aligned(4)));

typedef struct ParamStack ParamStack;

extern uint8_t runMem[OS_MAX_MEM] __attribute__((aligned(4)));
extern uint8_t exeMem[OS_MAX_TASK][OS_MAX_SGL_PG] __attribute__((aligned(4)));
#define OS_PHY_MEM_START runMem
#define OS_EXE_LOAD_START(i) exeMem[i]

void init_mem_info();
void ReadByteMem(uint8_t *Recv,uint8_t id);
int8_t findByteWithAddr(uint8_t id);
int32_t findIntWithAddr(uint8_t id);
float findFloatWithAddr(uint8_t id);
void setByte(int8_t byteText,uint8_t id);
void setInt(int32_t intText,uint8_t id);
void setFloat(float fText, uint8_t id);
int8_t ReArrangeMemAndTask(uint8_t id);
int8_t DelLastFuncMem(uint8_t id);
int8_t SuperFree(Magic *block);
int findFreeMemById(uint8_t id, int allocLen, int level);
uint8_t FindPhyMemOffByID(uint8_t id, uint32_t offset);
