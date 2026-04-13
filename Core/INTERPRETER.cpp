#include "INTERPRETER.h"
#include "Memory.h"
//extern from Memory.h
extern volatile uint8_t *CurPhyMem[OS_MAX_TASK];
extern volatile uint8_t *CurCmd[OS_MAX_TASK];
extern volatile uint8_t *MemTail[OS_MAX_TASK];
extern int16_t MemLevel[OS_MAX_TASK];
extern ParamStack Stack[OS_MAX_TASK][OS_MAX_PARAM];
extern uint8_t IndexOfSPS[OS_MAX_TASK];
#define INVALID_INPUT -10
#define OVERFLOW_ERR -11
#define INVALID_TYPE -12
int32_t CalcResu[OS_MAX_TASK] = { 0 };  //一些运算的运行结果寄存
int needJump[OS_MAX_TASK] = { 0 };
//字节码-函数映射表
typedef int8_t (*OpHandler)(uint8_t, int32_t *, uint16_t);
static OpHandler opcode_table[HLT + 1] = {
	[MOV] = mov,
	[EXT_BYTE] = ext_byte,
	[SETARRAY] = set_read_array,
	[READARRAY] = set_read_array,
	[INITARRAY] = init_array,
	[PUSH] = push,
	[PUSHP] = pushp,
	[ADD] = calc,
	[SUB] = calc,
	[MUL] = calc,
	[DIV] = calc,
	[EQ] = conds,
	[LT] = conds,
	[GT] = conds,
	[LE] = conds,
	[GE] = conds,
	[NE] = conds,
	[JMP] = jmp,
	[JMP_T] = jmp_t,
	[CALL] = call,
	[RET] = ret,
	[BIT_AOX] = bit_and_or_xor,
	[BIT_MOV] = bit_move,
	[ARS_TIMER] = ars_timer,
	[GPIO_WRITE] = gpio_write,
	[GPIO_READ] = gpio_read,
	[VAL] = val,
	[TO_INT] = to_int,
	[TO_FLOAT] = to_float,
	[HLT] = hlt
};

//可以叫赋值？
//MOV [地址][立即数或地址]
int8_t mov(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@mov ");
	Serial.print(taskId);
	Serial.print("\n");
	uint8_t type = (ParamType & 0x06) >> 1;
	ParamType &= 0x01;
	switch (type) {
		case 0:  //BYTE
			if (ParamType == 0) {
				FindPhyMemOffByID(taskId, params[0]);
				setByte((int8_t)params[1], taskId);
			} else {
				FindPhyMemOffByID(taskId, params[1]);
				int8_t x = findByteWithAddr(taskId);
				FindPhyMemOffByID(taskId, params[0]);
				setByte(x, taskId);
			}
			break;
		case 1:  //INT
			if (ParamType == 0) {
				FindPhyMemOffByID(taskId, params[0]);
				setInt((int32_t)params[1], taskId);
			} else {
				FindPhyMemOffByID(taskId, params[1]);
				int32_t x = findIntWithAddr(taskId);
				FindPhyMemOffByID(taskId, params[0]);
				setInt(x, taskId);
			}
			break;
		case 2:  //FLOAT
			if (ParamType == 0) {
				FindPhyMemOffByID(taskId, params[0]);
				float x = tranIntToFloat(params[1]);
				setFloat(x, taskId);
			} else {
				FindPhyMemOffByID(taskId, params[1]);
				float x = findFloatWithAddr(taskId);
				FindPhyMemOffByID(taskId, params[0]);
				setFloat(x, taskId);
			}
			break;
	}
}

int8_t ext_byte(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@ext_byte ");
	Serial.print(taskId);
	Serial.print("\n");
	FindPhyMemOffByID(taskId, params[1]);
	int8_t x = findByteWithAddr(taskId);
	FindPhyMemOffByID(taskId, params[0]);
	setInt(x, taskId);
}

//INIT_ARRAY [array] [count] [[1-byte tag(addr/imm)][param]...]
int8_t init_array(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@init_array ");
	Serial.print(taskId);
	Serial.print("\n");
	needJump[taskId] = 1;
	CurCmd[taskId] += 2 * sizeof(int32_t);
	uint8_t ivk_type = (ParamType & 0x06) >> 1;
	ParamType &= 0x01;
	int32_t count;
	if (ParamType == 0) {
		count = params[1];
	} else {
		FindPhyMemOffByID(taskId, params[1]);
		count = findIntWithAddr(taskId);
	}
	FindPhyMemOffByID(taskId, params[0]);
	int32_t i32, addr;
	float f32;
	for (int i = 0; i < count; i++) {
		uint8_t tag = *CurCmd[taskId]++;
		switch (ivk_type) {
			case 0:
				{
					if (!tag) {
						setByte(*CurCmd[taskId]++, taskId);
					} else {
						ARS_memmove(&addr, CurCmd[taskId], sizeof(int32_t));
						CurCmd[taskId] += sizeof(int32_t);
						FindPhyMemOffByID(taskId, addr);
						setByte(findByteWithAddr(taskId), taskId);
					}
					break;
				}
			case 1:
				{
					if (!tag) {
						ARS_memmove(&i32, CurCmd[taskId], sizeof(int32_t));
						setInt(i32, taskId);
						CurCmd[taskId] += sizeof(int32_t);
					} else {
						ARS_memmove(&addr, CurCmd[taskId], sizeof(int32_t));
						CurCmd[taskId] += sizeof(int32_t);
						FindPhyMemOffByID(taskId, addr);
						setInt(findIntWithAddr(taskId), taskId);
					}
					break;
				}
			case 2:
				{
					if (!tag) {
						ARS_memmove(&f32, CurCmd[taskId], sizeof(float));
						setFloat(f32, taskId);
						CurCmd[taskId] += sizeof(float);
					} else {
						ARS_memmove(&addr, CurCmd[taskId], sizeof(int32_t));
						CurCmd[taskId] += sizeof(int32_t);
						FindPhyMemOffByID(taskId, addr);
						setFloat(findFloatWithAddr(taskId), taskId);
					}
					break;
				}
		}
	}
}

//SET_ARRAY/READ_ARRAY [array] [index]
int8_t set_read_array(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId) {
	Serial.print("@set/read array ");
	Serial.print(taskId);
	Serial.print("\n");
	uint8_t cmd = cmdAndPmTp >> 3;
	uint8_t ivk_type = (cmdAndPmTp & 0x06) >> 1;
	cmdAndPmTp &= 0x01;
	int32_t index;
	if (cmdAndPmTp == 0) {
		index = params[1];
	} else {
		FindPhyMemOffByID(taskId, params[1]);
		index = findIntWithAddr(taskId);
	}
	FindPhyMemOffByID(taskId, params[0]);
	CurPhyMem[taskId] += (!ivk_type ? 1 : 4) * index;
	switch (ivk_type) {
		case 0:
			{
				if (cmd == READARRAY)
					CalcResu[taskId] = findByteWithAddr(taskId);
				else
					setByte(CalcResu[taskId], taskId);
				break;
			}
		case 1:
			{
				if (cmd == READARRAY)
					CalcResu[taskId] = findIntWithAddr(taskId);
				else
					setInt(CalcResu[taskId], taskId);
				break;
			}
		case 2:
			{
				if (cmd == READARRAY)
					CalcResu[taskId] = tranFloatToInt(findFloatWithAddr(taskId));
				else {
					float x = tranIntToFloat(CalcResu[taskId]);
					setFloat(x, taskId);
				}
				break;
			}
	}
	Serial.print("CalcResu: ");
	Serial.print(CalcResu[taskId]);
	Serial.print("\n");
}

// VAL [立即数或地址]
// 根据类型和地址标志，将值存入 CalcResu[taskId]
int8_t val(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@val ");
	Serial.print(taskId);
	Serial.print("\n");
	uint8_t type = (ParamType & 0x06) >> 1;  // 类型：0=B,1=I,2=F
	uint8_t is_addr = ParamType & 0x01;      // 0=立即数，1=地址
	if (is_addr) {
		FindPhyMemOffByID(taskId, params[0]);
		if (type == 0) {
			CalcResu[taskId] = findByteWithAddr(taskId);  // 字节符号扩展
		} else if (type == 1) {
			CalcResu[taskId] = findIntWithAddr(taskId);
		} else {  // float
			float f = findFloatWithAddr(taskId);
			CalcResu[taskId] = tranFloatToInt(f);
		}
	} else {
		// 立即数
		if (type == 0) {
			CalcResu[taskId] = (int8_t)params[0];
		} else if (type == 1) {
			CalcResu[taskId] = params[0];
		} else {  // float
			float f = tranIntToFloat(params[0]);
			CalcResu[taskId] = tranFloatToInt(f);
		}
	}
	return 0;
}

// TO_INT [地址]：将地址处（float）转换为 int 存入 CalcResu
int8_t to_int(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@to_int ");
	Serial.print(taskId);
	Serial.print("\n");
	// 参数只有一个地址，忽略 ParamType（无类型指定）
	FindPhyMemOffByID(taskId, params[0]);
	float f = findFloatWithAddr(taskId);
	CalcResu[taskId] = (int32_t)f;  // 浮点转整数
	return 0;
}

// TO_FLOAT [地址]：将地址处（int）转换为 float 存入 CalcResu
int8_t to_float(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@to_float ");
	Serial.print(taskId);
	Serial.print("\n");
	FindPhyMemOffByID(taskId, params[0]);
	int32_t i = findIntWithAddr(taskId);
	float f = (float)i;
	// 将浮点数按 IEEE754 位模式存入 CalcResu（CalcResu 是 int32_t，需保持位模式）
	ARS_memmove(&CalcResu[taskId], &f, sizeof(float));
	return 0;
}

//将CalcResu存入内存
//PUSH [内存地址]
int8_t push(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@push ");
	Serial.print(taskId);
	Serial.print("\n");
	FindPhyMemOffByID(taskId, params[0]);
	//保存为BYTE
	if (ParamType == 0) {
		setByte((int8_t)CalcResu[taskId], taskId);
		//保存为INT？我不知道四字节的变量怎么称呼
	} else if (ParamType == 1) {
		setInt((int32_t)CalcResu[taskId], taskId);
	} else {
		setFloat((float)CalcResu[taskId], taskId);
	}
}

//通过millis函数获取系统时钟并存入内存
//ARS_TIMER [内存地址]
int8_t ars_timer(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@timer ");
	Serial.print(taskId);
	Serial.print("\n");
	FindPhyMemOffByID(taskId, params[0]);
	setInt((int32_t)millis(), taskId);
}

int8_t gpio_write(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@gpio_write ");
	Serial.print(taskId);
	Serial.print("\n");
	ParamType &= 0x07;
	int pin, val;
	if (ParamType == 0) {  // 两个立即数
		pin = params[0];
		val = params[1];
	} else if (ParamType == 1) {  // 参数1为地址，参数2为立即数
		FindPhyMemOffByID(taskId, params[0]);
		pin = findIntWithAddr(taskId);
		val = params[1];
	} else if (ParamType == 2) {  // 参数2为地址，参数1为立即数
		FindPhyMemOffByID(taskId, params[1]);
		val = findIntWithAddr(taskId);
		pin = params[0];
	} else if (ParamType == 3) {  // 两个参数均为地址
		FindPhyMemOffByID(taskId, params[0]);
		pin = findIntWithAddr(taskId);
		FindPhyMemOffByID(taskId, params[1]);
		val = findIntWithAddr(taskId);
	}
	pinMode((uint8_t)pin, OUTPUT);
	digitalWrite((uint8_t)pin, (uint8_t)val);
	return 0;
}

int8_t gpio_read(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@gpio_read ");
	Serial.print(taskId);
	Serial.print("\n");
	ParamType &= 0x01;
	int readPin;
	if (ParamType == 0) {
		readPin = digitalRead(params[1]);
	} else {
		FindPhyMemOffByID(taskId, params[1]);
		readPin = digitalRead(findByteWithAddr(taskId));
	}
	pinMode((uint8_t)readPin, INPUT);
	FindPhyMemOffByID(taskId, params[0]);
	setInt(readPin, taskId);
}

//子程序参数栈压入参数
//PUSHP [子程序参数的内存地址或立即数]
int8_t pushp(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("@pushp ");
	Serial.print(taskId);
	Serial.print("\n");
	uint8_t p = ParamType & 0x01;
	ParamType = (ParamType & 0x06) >> 1;
	if (p == 0) {
		if (ParamType == 0) {
			Stack[taskId][IndexOfSPS[taskId]].DATA.BYTE = (int8_t)params[0];
			Stack[taskId][IndexOfSPS[taskId]].Type = 1;
		} else if (ParamType == 1) {
			Stack[taskId][IndexOfSPS[taskId]].DATA.INT = (int32_t)params[0];
			Stack[taskId][IndexOfSPS[taskId]].Type = 2;
		} else {
			Stack[taskId][IndexOfSPS[taskId]].DATA.FLOAT = tranIntToFloat(params[0]);
			Stack[taskId][IndexOfSPS[taskId]].Type = 3;
		}
	} else {
		FindPhyMemOffByID(taskId, params[0]);
		if (ParamType == 0) {
			Stack[taskId][IndexOfSPS[taskId]].DATA.BYTE = findByteWithAddr(taskId);
			Stack[taskId][IndexOfSPS[taskId]].Type = 1;
		} else if (ParamType == 1) {
			Stack[taskId][IndexOfSPS[taskId]].DATA.INT = findIntWithAddr(taskId);
			Stack[taskId][IndexOfSPS[taskId]].Type = 2;
		} else {
			Stack[taskId][IndexOfSPS[taskId]].DATA.FLOAT = findFloatWithAddr(taskId);
			Stack[taskId][IndexOfSPS[taskId]].Type = 3;
		}
	}
	IndexOfSPS[taskId]++;
	if (IndexOfSPS[taskId] >= OS_MAX_PARAM) {
		return OUT_PARAM_BOUND;
	}
}

//volatile uint8_t *p;
//调用子程序
//CALL [子程序编号，在编译过程中确定]
int8_t call(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	needJump[taskId] = 1;
	Serial.print("@call ");
	Serial.print(taskId);
	//内存层级+1
	MemLevel[taskId]++;
	//保存上下文数据
	//在rp2040这种32位环境下指针大小为4，与int大小一致
	//uint32_t CurAddrOfMemPtr = CurPhyMem[taskId] - OS_PHY_MEM_START;
	uint32_t CurAddrOfCmd = CurCmd[taskId] - OS_EXE_LOAD_START(taskId);
	//程序命令指针指向参数所表示的地址
	uint32_t tmpCurCmd = (uint32_t)params[0];
	Serial.print("addr ");
	Serial.print(tmpCurCmd);
	Serial.print("\n");
	//前四个字节代表运行所需内存总大小（包括行参）
	//这个值在编译过程中确定
	uint32_t ReqMemSize;
	memcpy(&ReqMemSize, (void *)(OS_EXE_LOAD_START(taskId) + tmpCurCmd), 4);
	//跳过四字节进入程序主体
	tmpCurCmd += sizeof(uint32_t);
	//分配内存
	//分配完后CurPhyMem[taskId]跳转至分配的内存的首地址
	findFreeMemById(taskId, ReqMemSize, MemLevel[taskId]);
	CurCmd[taskId] = (volatile uint8_t *)(OS_EXE_LOAD_START(taskId) + tmpCurCmd);
	//p = CurCmd[taskId];
	//show_mem_msg((Magic *)CurPhyMem[taskId]);
	//暂存当前内存指针指向地址
	//volatile uint8_t *tmp_ptr = CurPhyMem[taskId];
	//跳过魔术字头
	CurPhyMem[taskId] += sizeof(Magic);
	//压入上下文数据
	//*(volatile uint32_t *)CurPhyMem[taskId] = CurAddrOfMemPtr;
	memcpy((void *)CurPhyMem[taskId], &CurAddrOfCmd, 4);
	CurPhyMem[taskId] += sizeof(int32_t);
	int i;
	//压入参数
	for (i = 0; i < OS_MAX_PARAM; i++) {
		if (Stack[taskId][i].Type) {
			if (Stack[taskId][i].Type == 1) {
				setByte(Stack[taskId][i].DATA.BYTE, taskId);
			} else if (Stack[taskId][i].Type == 2) {
				setInt(Stack[taskId][i].DATA.INT, taskId);
			} else {
				setFloat(Stack[taskId][i].DATA.FLOAT, taskId);
			}
			//Type=0表示从这儿开始往后的参数栈都未启用
			//参数栈的数据紧密相连，不存在跳跃
		} else {
			break;
		}
	}
	//销毁参数栈的形参
	for (int j = 0; j < i; j++) {
		Stack[taskId][j].Type = 0;
	}
	IndexOfSPS[taskId] = 0;
	//返回内存头
	//CurPhyMem[taskId] = tmp_ptr;
}

//子程序返回上文
//无参数
int8_t ret(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	needJump[taskId] = 1;
	//提取上下文信息
	Serial.print("@ret ");
	Serial.print(taskId);
	Serial.print("\n");
	//uint32_t Mem = *(volatile int32_t *)(MemTail[taskId] + sizeof(Magic));
	uint32_t Cmd;
	memcpy(&Cmd, (void *)(MemTail[taskId] + sizeof(Magic)), 4);
	//销毁变量（内存层级在该函数中自减）
	DelLastFuncMem(taskId);
	//跳回上文
	CurCmd[taskId] = (volatile uint8_t *)(Cmd + OS_EXE_LOAD_START(taskId));
}

//条件判断
int8_t conds(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId) {
	Serial.print("@conds ");
	Serial.print(taskId);
	// 前五位代表命令
	uint8_t cmd = cmdAndPmTp >> 3;
	// 后三位共同代表参数的一些性质
	uint8_t ParamType = cmdAndPmTp & 0x07;
	// 解析参数类型和数据大小
	//用double来覆盖所有类型最大可表示的值
	//简化操作
	double val1, val2;
	//x用来判断参数是不是float类型（ParamType第三位）
	uint8_t x = (ParamType & 0x04) >> 2;
	//无论如何去掉第三位，否则可能参数误判
	ParamType &= 0x03;
	// 读取参数1的值（根据ParamType）
	if (ParamType == 0) {  // 两个立即数
		val1 = x ? tranIntToFloat(params[0]) : params[0];
		val2 = x ? tranIntToFloat(params[1]) : params[1];
	} else if (ParamType == 1) {  // 参数1为地址，参数2为立即数
		// 从内存读取参数1的值
		FindPhyMemOffByID(taskId, params[0]);
		val1 = x ? findFloatWithAddr(taskId) : findIntWithAddr(taskId);
		// 立即数参数2处理
		val2 = x ? tranIntToFloat(params[1]) : params[1];
	} else if (ParamType == 2) {  // 参数2为地址，参数1为立即数
		// 从内存读取参数2的值
		FindPhyMemOffByID(taskId, params[1]);
		val2 = x ? findFloatWithAddr(taskId) : findIntWithAddr(taskId);
		// 立即数参数1处理
		val1 = x ? tranIntToFloat(params[0]) : params[0];
	} else if (ParamType == 3) {  // 两个参数均为地址
		// 读取参数1的地址
		FindPhyMemOffByID(taskId, params[0]);
		val1 = x ? findFloatWithAddr(taskId) : findIntWithAddr(taskId);
		// 读取参数2的地址
		FindPhyMemOffByID(taskId, params[1]);
		val2 = x ? findFloatWithAddr(taskId) : findIntWithAddr(taskId);
	}
	// 根据指令进行比较
	switch (cmd) {
		case EQ:
			CalcResu[taskId] = (val1 == val2);
			break;
		case LT:
			CalcResu[taskId] = (val1 < val2);
			break;
		case GT:
			CalcResu[taskId] = (val1 > val2);
			break;
		case LE:
			{
				CalcResu[taskId] = (val1 <= val2);
			}
			break;
		case GE:
			CalcResu[taskId] = (val1 >= val2);
			break;
		case NE:
			CalcResu[taskId] = (val1 != val2);
			break;
		default:
			return -1;  // 非法指令
	}
	Serial.print(CalcResu[taskId]);
	Serial.print("\n");
}

//无条件跳转
int8_t jmp(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	needJump[taskId] = 1;
	Serial.print("@jmp ");
	Serial.print(taskId);
	CurCmd[taskId] = (volatile uint8_t *)(OS_EXE_LOAD_START(taskId) + params[0]);
	Serial.print("to ");
	Serial.print(params[0]);
	Serial.print("\n");
}

//情况成立（CalcResu不为0）跳转
int8_t jmp_t(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	needJump[taskId] = 1;
	Serial.print("@jmp_t ");
	Serial.print(taskId);
	if (CalcResu[taskId]) CurCmd[taskId] = (volatile uint8_t *)(OS_EXE_LOAD_START(taskId) + params[0]);
	Serial.print("to ");
	Serial.print(params[0]);
	Serial.print("is_true ");
	Serial.print(CalcResu[taskId]);
	Serial.print("\n");
}

//加减乘除
int8_t calc(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId) {
	Serial.print("@calc ");
	Serial.print(taskId);
	uint8_t cmd = cmdAndPmTp >> 3;
	uint8_t ParamType = cmdAndPmTp & 0x07;
	// 新增参数类型标识：ParamType 的第三位表示是否为浮点运算 (1=float)
	uint8_t is_float = (ParamType & 0x04) >> 2;  // 取第三位
	ParamType &= 0x03;                           // 保留原始参数类型
	float val1_f, val2_f, result_f;
	int32_t val1_i, val2_i, result_i;
	// 根据参数类型读取操作数（支持立即数、地址、混合类型）
	if (is_float) {
		// 处理浮点运算
		if (ParamType == 0) {  // 两个立即数（需将 int32_t 转换为 float）
			val1_f = tranIntToFloat(params[0]);
			val2_f = tranIntToFloat(params[1]);
		} else if (ParamType == 1) {  // 参数1为地址，参数2为立即数
			FindPhyMemOffByID(taskId, params[0]);
			val1_f = findFloatWithAddr(taskId);
			val2_f = tranIntToFloat(params[1]);
		} else if (ParamType == 2) {  // 参数2为地址，参数1为立即数
			FindPhyMemOffByID(taskId, params[1]);
			val2_f = findFloatWithAddr(taskId);
			val1_f = tranIntToFloat(params[0]);
		} else if (ParamType == 3) {  // 两个参数均为地址
			FindPhyMemOffByID(taskId, params[0]);
			val1_f = findFloatWithAddr(taskId);
			FindPhyMemOffByID(taskId, params[1]);
			val2_f = findFloatWithAddr(taskId);
		}
		// 执行浮点运算
		switch (cmd) {
			case ADD:
				result_f = val1_f + val2_f;
				break;
			case SUB:
				result_f = val1_f - val2_f;
				break;
			case MUL:
				result_f = val1_f * val2_f;
				break;
			case DIV:
				if (val2_f == 0.0f) return DIV_BY_0;
				result_f = val1_f / val2_f;
				break;
		}
		// 将结果转换为 int32_t 存入 CalcResu（需确保内存对齐）
		ARS_memmove(&CalcResu[taskId], &result_f, sizeof(float));
	} else {
		// 原有整数运算逻辑（略作调整）
		if (ParamType == 0) {
			val1_i = params[0];
			val2_i = params[1];
		} else if (ParamType == 1) {
			FindPhyMemOffByID(taskId, params[0]);
			val1_i = findIntWithAddr(taskId);
			val2_i = params[1];
		} else if (ParamType == 2) {
			FindPhyMemOffByID(taskId, params[1]);
			val2_i = findIntWithAddr(taskId);
			val1_i = params[0];
		} else if (ParamType == 3) {
			FindPhyMemOffByID(taskId, params[0]);
			val1_i = findIntWithAddr(taskId);
			FindPhyMemOffByID(taskId, params[1]);
			val2_i = findIntWithAddr(taskId);
		}
		switch (cmd) {
			case ADD:
				result_i = val1_i + val2_i;
				//Serial.print("@add result:%d\n", result_i);
				break;
			case SUB:
				result_i = val1_i - val2_i;
				//Serial.print("@sub result:%d\n", result_i);
				break;
			case MUL:
				result_i = val1_i * val2_i;
				break;
			case DIV:
				if (val2_i == 0) return DIV_BY_0;
				result_i = val1_i / val2_i;
				break;
		}
		CalcResu[taskId] = result_i;
	}
	if (!is_float)
		Serial.print(CalcResu[taskId]);
	else
		Serial.print(tranIntToFloat(CalcResu[taskId]));
	Serial.print("\n");
}

int8_t hlt(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	Serial.print("\nExecuted successfully! ");
	Serial.print(taskId);
	Serial.print("\n");
	ReArrangeMemAndTask(taskId);
}

int8_t bit_and_or_xor(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	int32_t val1_i, val2_i;
	if (ParamType == 0) {
		val1_i = params[1];
		val2_i = params[2];
	} else if (ParamType == 1) {
		FindPhyMemOffByID(taskId, params[1]);
		val1_i = findIntWithAddr(taskId);
		val2_i = params[2];
	} else if (ParamType == 2) {
		FindPhyMemOffByID(taskId, params[1]);
		val1_i = findIntWithAddr(taskId);
		FindPhyMemOffByID(taskId, params[2]);
		val2_i = findIntWithAddr(taskId);
	}
	switch (params[0]) {
		case 1:
			val1_i &= val2_i;
			break;
		case 2:
			val1_i |= val2_i;
			break;
		case 3:
			val1_i ^= val2_i;
			break;
	}
	CalcResu[taskId] = val1_i;
}

int8_t bit_move(uint8_t ParamType, int32_t *params, uint16_t taskId) {
	int32_t val1_i, val2_i;
	uint8_t operate = (ParamType & 0x04) >> 2;
	ParamType &= 0x03;
	if (ParamType == 0) {
		val1_i = params[0];
		val2_i = params[1];
	} else if (ParamType == 1) {
		FindPhyMemOffByID(taskId, params[0]);
		val1_i = findIntWithAddr(taskId);
		val2_i = params[1];
	} else if (ParamType == 2) {
		FindPhyMemOffByID(taskId, params[0]);
		val1_i = findIntWithAddr(taskId);
		FindPhyMemOffByID(taskId, params[1]);
		val2_i = findIntWithAddr(taskId);
	}
	switch (operate) {
		case 0:
			val1_i <<= val2_i;
			break;
		case 1:
			val1_i >>= val2_i;
			break;
	}
	CalcResu[taskId] = val1_i;
}

//注意！！
//params并不代表它一定表示的是int类型
//可能是与float类型共用相同的四字节内存
int8_t interprete(uint8_t cmdAndPmTp, int32_t *params, uint16_t taskId) {
	//前五个字节代表命令
	uint8_t cmd = cmdAndPmTp >> 3;
	//后三个字节共同代表参数的一些性质
	uint8_t ParamType = cmdAndPmTp & 0x07;
	if (!((cmd >= ADD && cmd <= NE) || cmd == SETARRAY || cmd == READARRAY)) {
		opcode_table[cmd](ParamType, params, taskId);
	} else {
		opcode_table[cmd](cmdAndPmTp, params, taskId);
	}
}
