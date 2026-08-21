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


#define SPLIT "SPLT"  //魔术字：已被程序占位
#define FREE "FREE"   //魔术字：程序内存已被清空
#define CHECK 1145141919
#endif

struct Magic {
	char MagicHead[4];
	uars_i8 id;
	ars_i32 Check;
	//Check为守卫标识
	//最后的内存防线
	//若连此值都被破坏则认为该段内存完全损坏
	uars_i32 len;
	//len=子程序运行所需内存+4字节调用时命令内存指针指向地址
	//不包括魔术字头
	volatile uars_i8 *last_block;
	volatile uars_i8 *next_block;
} __attribute__((packed));

typedef struct Magic Magic;

struct Mail{
	ars_i32 content;
	uars_i8 flag;
};

typedef struct Mail Mail;

extern uars_i8 runMem[OS_MAX_MEM] __attribute__((aligned(4)));
extern uars_i8 exeMem[OS_MAX_TASK][OS_MAX_SGL_PG] __attribute__((aligned(4)));
#define OS_PHY_MEM_START runMem
#define OS_EXE_LOAD_START(i) exeMem[i]

void init_mem_info();
void ReadByteMem(uars_i8 *Recv, uars_i8 id);
ars_i8 findByteWithAddr(uars_i8 id);
ars_i32 findIntWithAddr(uars_i8 id);
float findFloatWithAddr(uars_i8 id);
void setByte(ars_i8 byteText, uars_i8 id);
void setInt(ars_i32 intText, uars_i8 id);
void setFloat(float fText, uars_i8 id);
ars_i8 ReArrangeMemAndTask(uars_i8 id);
ars_i8 DelLastFuncMem(uars_i8 id);
ars_i8 SuperFree(Magic *block);
int findFreeMemById(uars_i8 id, int allocLen, int level);
uars_i8 FindPhyMemOffByID(uars_i8 id, uars_i32 offset);
