#include "Compiler.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char codePool[65536] = {0};
//首四个字节用来存放main的入口
unsigned char *ptr = codePool + 4;
int entry_point = 0;
char need_mem_init = 0, start_mem = 0;
char current_func_name[MAX_TOKEN_LEN] = {0};
// 当前子程序的符号表
Symbol symbols[MAX_SYMBOLS];
int symbol_count = 0;
int ttl_var_len = 0;          // 当前子程序总变量长度

CompilerState state = {0};
int level_count = 0;
char result[MAX_TOKEN_LEN] = {0};

// 辅助函数：跳过空白字符
char *skip_blank(char *sentence) {
	while (*sentence == ' ' || *sentence == '\t') sentence++;
	return sentence;
}

// 读取一个token，支持引号字符串
char *read_a_token(char *sentence) {
	memset(result, 0, MAX_TOKEN_LEN);
	char *p = result, *q = sentence;
	if (*q != '"') {
		while (*q && *q != ' ' && *q != '\t')
			*p++ = *q++;
		return q;
	} else {
		*p++ = *q++;
		while (*q && *q != '"')
			*p++ = *q++;
		if (*q == '"') *p++ = *q++;
		return q;
	}
}

// 获取变量偏移地址
int return_addr_of_var(char *name) {
	for (int i = 0; i < symbol_count; i++) {
		if (!strcmp(name, symbols[i].name)) {
			return symbols[i].addr;
		}
	}
	printf("Error: undefined variable %s\n", name);
	exit(1);
}

// 字节码生成函数（与解释器匹配）
void generate_1(char instruction, char instruct_type, char param_type, int *params, short count) {
	char instruct = instruction << 3;
	instruct |= (instruct_type << 1);
	instruct |= param_type;
	*ptr++ = instruct;
	for (int i = 0; i < count; i++) {
		*(int32_t *)ptr = params[i];
		ptr += 4;
	}
}

void generate_2(char instruction, char is_float, char param_type, int *params) {
	char instruct = instruction << 3;
	instruct |= (is_float << 2);
	instruct |= param_type;
	*ptr++ = instruct;
	*(int32_t *)ptr = params[0];
	ptr += 4;
	*(int32_t *)ptr = params[1];
	ptr += 4;
}

void generate_3(char instruction, int *params, short count) {
	char instruct = instruction << 3;
	*ptr++ = instruct;
	for (int i = 0; i < count; i++) {
		*(int32_t *)ptr = params[i];
		ptr += 4;
	}
}

void generate_4(char instruction, char param_type, int *params, short count) {
	char instruct = instruction << 3;
	instruct |= param_type;
	*ptr++ = instruct;
	for (int i = 0; i < count; i++) {
		*(int32_t *)ptr = params[i];
		ptr += 4;
	}
}

// 处理 MEM 段内的变量声明
void create_sub(char *sentence) {
	char *s = sentence;
	s = read_a_token(s);
	s = skip_blank(s);

	if (!strcmp(result, "mem")) {
		start_mem = 1;
		return;
	}
	if (!strcmp(result, "end_mem")) {
		start_mem = 0;
		need_mem_init = 0;
		for (int i = 0; i < symbol_count; i++) {
			printf("FuncName:%s VarName:%s Offset:%d\n", current_func_name, symbols[i].name, symbols[i].addr);
		}
		printf("Total memory length of %s:%d\n\n", current_func_name, ttl_var_len);
		// 回填内存大小到子程序开头（预留的4字节）
		memcpy(ptr, &ttl_var_len, 4);
		ptr += 4;
		return;
	}

	if (start_mem && need_mem_init) {
		// 变量名
		strcpy(symbols[symbol_count].name, result);
		symbols[symbol_count].addr = ttl_var_len;
		s = read_a_token(s);
		s = skip_blank(s);
		int elem_size = 0;
		if (!strcmp(result, "B")) elem_size = 1;
		else if (!strcmp(result, "I") || !strcmp(result, "F")) elem_size = 4;
		else {
			printf("Error: unknown type %s\n", result);
			exit(1);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		int array_len = atoi(result);
		ttl_var_len += elem_size * (array_len + 1);
		symbol_count++;
	}
}

// 编译一行代码
void compile_one_line(char *sentence) {
	if (need_mem_init) {
		create_sub(sentence);
		return;
	}

	char *s = sentence;
	s = read_a_token(s);
	s = skip_blank(s);

	// 忽略空行和注释
	if (result[0] == '\0' || result[0] == ';')
		return;

	// 处理 MAIN 或 FN 开始
	if (!strcmp(result, "main") || !strcmp(result, "fn")) {
		if (!strcmp(result, "main")) {
			entry_point = ptr - codePool;
			memcpy(codePool, &entry_point, 4);
			strcpy(current_func_name, "main");
			printf("Addr of function main:%d\n", entry_point);
		} else {
			// FN: 记录函数名和地址（内存大小字段的地址）
			s = read_a_token(s);
			s = skip_blank(s);
			strcpy(state.functions[state.func_count].name, result);
			strcpy(current_func_name, result);
			state.functions[state.func_count].addr = (uint32_t)(ptr - codePool);
			state.func_count++;
			printf("Addr of function %s:%d\n", result, (int)(ptr - codePool));
		}
		need_mem_init = 1;
		level_count++;
		return;
	}

	// 处理标签定义
	if (!strcmp(result, "lb")) {
		s = read_a_token(s);
		s = skip_blank(s);
		strcpy(state.labels[state.label_count].name, result);
		state.labels[state.label_count].addr = (uint32_t)(ptr - codePool);
		state.label_count++;
		printf("Label %s:%d\n", result, (int)(ptr - codePool));
		return;
	}

	// 处理 MOV 指令
	if (!strcmp(result, "mov")) {
		s = read_a_token(s);
		s = skip_blank(s);
		char type;
		if (!strcmp(result, "B")) type = 0;
		else if (!strcmp(result, "I")) type = 1;
		else if (!strcmp(result, "F")) type = 2;
		else {
			printf("Error: MOV missing type\n");
			exit(1);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		int dest = return_addr_of_var(result);
		s = read_a_token(s);
		s = skip_blank(s);
		int src_is_addr = (result[0] == '$');
		int src_val;
		if (src_is_addr) {
			src_val = return_addr_of_var(result);
		} else {
			if (type != 2)
				src_val = atoi(result);
			else {
				float f = atof(result);
				memcpy(&src_val, &f, sizeof(float));
			}
		}
		int params[2] = {dest, src_val};
		generate_1(MOV, type, src_is_addr, params, 2);
		return;
	}
// 处理 ext_byte 指令
	if (!strcmp(result, "ext_byte")) {
		s = read_a_token(s);
		s = skip_blank(s);
		int dest = return_addr_of_var(result); // 目标地址
		s = read_a_token(s);
		s = skip_blank(s);
		int src = return_addr_of_var(result);  // 源地址（字节）
		int params[2] = {dest, src};
		// 指令格式：高5位 EXT_BYTE，低3位任意（这里填0）
		generate_1(EXT_BYTE, 0, 0, params, 2);
		return;
	}

	// 处理 set_array 和 read_array 指令
	if (!strcmp(result, "set_array") || !strcmp(result, "read_array")) {
		int is_set = !strcmp(result, "set_array"); // 1=set, 0=read
		// 读取类型 B/I/F
		s = read_a_token(s);
		s = skip_blank(s);
		int ivk_type;
		if (!strcmp(result, "B")) ivk_type = 0;
		else if (!strcmp(result, "I")) ivk_type = 1;
		else if (!strcmp(result, "F")) ivk_type = 2;
		else {
			printf("Error: set_array/read_array missing type (B/I/F)\n");
			exit(1);
		}
		// 读取数组基地址
		s = read_a_token(s);
		s = skip_blank(s);
		int array_addr = return_addr_of_var(result);
		// 读取索引
		s = read_a_token(s);
		s = skip_blank(s);
		int idx_is_addr = (result[0] == '$');
		int idx_val;
		if (idx_is_addr) {
			idx_val = return_addr_of_var(result);
		} else {
			idx_val = atoi(result);
		}
		int params[2] = {array_addr, idx_val};
		uint8_t opcode = is_set ? SETARRAY : READARRAY;
		generate_1(opcode, ivk_type, idx_is_addr, params, 2);
		return;
	}

	// 处理 VAL 指令
	if (!strcmp(result, "val")) {
		s = read_a_token(s);
		s = skip_blank(s);
		// 读取类型 B/I/F
		int type_flag;
		if (!strcmp(result, "B")) type_flag = 0;
		else if (!strcmp(result, "I")) type_flag = 1;
		else if (!strcmp(result, "F")) type_flag = 2;
		else {
			printf("Error: VAL missing type (B/I/F)\n");
			exit(1);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		int is_addr = (result[0] == '$');
		int val;
		if (is_addr) {
			val = return_addr_of_var(result);
		} else {
			if (type_flag != 2)
				val = atoi(result);
			else {
				float f = atof(result);
				memcpy(&val, &f, sizeof(float));
			}
		}
		int params[1] = {val};
		generate_1(VAL, type_flag, is_addr, params, 1);
		return;
	}

	// 处理 init_array 指令
	if (!strcmp(result, "init_array")) {
		// 读取类型 B/I/F
		s = read_a_token(s);
		s = skip_blank(s);
		int ivk_type;
		if (!strcmp(result, "B")) ivk_type = 0;
		else if (!strcmp(result, "I")) ivk_type = 1;
		else if (!strcmp(result, "F")) ivk_type = 2;
		else {
			printf("Error: init_array missing type (B/I/F)\n");
			exit(1);
		}
		// 读取数组基地址
		s = read_a_token(s);
		s = skip_blank(s);
		int array_addr = return_addr_of_var(result);
		// 读取 count（可以是立即数或地址）
		s = read_a_token(s);
		s = skip_blank(s);
		int count_is_addr = (result[0] == '$');
		int count_val;
		if (count_is_addr) {
			count_val = return_addr_of_var(result);
		} else {
			count_val = atoi(result);
		}
		// 生成指令头部（两个固定参数）
		int params[2] = {array_addr, count_val};
		generate_1(INITARRAY, ivk_type, count_is_addr, params, 2);
		// 现在处理可变参数列表
		while (1) {
			// 跳过空白
			s = skip_blank(s);
			if (*s == '\0' || *s == '\n' || *s == ';') break;
			s = read_a_token(s);
			int is_addr = (result[0] == '$');
			int val;
			if (is_addr) {
				val = return_addr_of_var(result);
			}
			// 写入 tag 字节 (0=立即数,1=地址)
			*ptr++ = is_addr ? 1 : 0;
			if (ivk_type == 0) { // BYTE
				if (is_addr) {
					*(int32_t *)ptr = val;
					ptr += 4;
				} else {
					*ptr++ = (uint8_t)atoi(result);
				}
			} else if (ivk_type == 1) {
				if (is_addr) {
					*(int32_t *)ptr = val;
					ptr += 4;
				} else {
					*(int32_t *)ptr = atoi(result);
					ptr += 4;
				}
			} else {
				if (is_addr) {
					*(int32_t *)ptr = val;
					ptr += 4;
				} else {
					*(float *)ptr = atof(result);
					ptr += 4;
				}
			}
		}
		return;
	}
	
	// 处理 TO_INT 和 TO_FLOAT
	if (!strcmp(result, "to_int") || !strcmp(result, "to_float")) {
		int op = (!strcmp(result, "to_int")) ? TO_INT : TO_FLOAT;
		s = read_a_token(s);
		s = skip_blank(s);
		if (result[0] != '$') {
			printf("Error: %s requires a variable address (with $)\n", result);
			exit(1);
		}
		int addr = return_addr_of_var(result);
		int params[1] = {addr};
		generate_3(op, params, 1);  // 使用无类型标志的生成函数
		return;
	}
	
	// 处理 GPIO_READ 指令
	if (!strcmp(result, "gpio_read")) {
		s = read_a_token(s);
		s = skip_blank(s);
		int dest = return_addr_of_var(result);
		s = read_a_token(s);
		s = skip_blank(s);
		int src_is_addr = (result[0] == '$');
		int src_val;
		if (src_is_addr) {
			src_val = return_addr_of_var(result);
		} else {
			src_val = atoi(result);
		}
		int params[2] = {dest, src_val};
		generate_1(GPIO_READ, 0, src_is_addr, params, 2);
		return;
	}

	// 处理 GPIO_WRITE 指令
	if (!strcmp(result, "gpio_write")) {
		int p1[2] = {0, 0}; // p1[0]是否地址，p1[1]值
		int p2[2] = {0, 0};
		s = read_a_token(s);
		s = skip_blank(s);
		if (result[0] == '$') {
			p1[0] = 1;
			p1[1] = return_addr_of_var(result);
		} else {
			p1[1] = atoi(result);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		if (result[0] == '$') {
			p2[0] = 1;
			p2[1] = return_addr_of_var(result);
		} else {
			p2[1] = atoi(result);
		}
		char param_type;
		if (!p1[0] && !p2[0]) param_type = 0;
		else if (p1[0] && !p2[0]) param_type = 1;
		else if (!p1[0] && p2[0]) param_type = 2;
		else param_type = 3;
		int params[2] = {p1[1], p2[1]};
		generate_4(GPIO_WRITE, param_type, params, 2);
		return;
	}

	// 处理算术/比较指令
	static const char *ops[] = {"add", "sub", "mul", "div", "eq", "lt", "gt", "le", "ge", "ne"};
	int op_idx;
	for (op_idx = 0; op_idx < 10; op_idx++)
		if (!strcmp(result, ops[op_idx])) break;
	if (op_idx < 10) {
		s = read_a_token(s);
		s = skip_blank(s);
		char is_float = 0;
		if (!strcmp(result, "I")) is_float = 0;
		else if (!strcmp(result, "F")) is_float = 1;
		else {
			printf("Error: missing type for arithmetic\n");
			exit(1);
		}
		// 读取两个操作数
		int p1[2] = {0, 0}; // p1[0]是否地址，p1[1]值
		int p2[2] = {0, 0};
		s = read_a_token(s);
		s = skip_blank(s);
		if (result[0] == '$') {
			p1[0] = 1;
			p1[1] = return_addr_of_var(result);
		} else {
			if (!is_float)
				p1[1] = atoi(result);
			else {
				float f = atof(result);
				memcpy(&p1[1], &f, sizeof(float));
			}
		}
		s = read_a_token(s);
		s = skip_blank(s);
		if (result[0] == '$') {
			p2[0] = 1;
			p2[1] = return_addr_of_var(result);
		} else {
			if (!is_float)
				p2[1] = atoi(result);
			else {
				float f = atof(result);
				memcpy(&p2[1], &f, sizeof(float));
			}
		}

		// 确定 param_type
		char param_type;
		if (!p1[0] && !p2[0]) param_type = 0;
		else if (p1[0] && !p2[0]) param_type = 1;
		else if (!p1[0] && p2[0]) param_type = 2;
		else param_type = 3;

		int params[2] = {p1[1], p2[1]};
		generate_2(ADD + op_idx, is_float, param_type, params);
		return;
	}

	// 处理 PUSHP
	if (!strcmp(result, "pushp")) {
		s = read_a_token(s);
		s = skip_blank(s);
		char type_flag; // 0:立即数 1:地址
		if (!strcmp(result, "I")) type_flag = 1;
		else if (!strcmp(result, "F")) type_flag = 2;
		else if (!strcmp(result, "B")) type_flag = 0;
		else {
			printf("Error: PUSHP missing type\n");
			exit(1);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		int is_addr = (result[0] == '$');
		int val;
		if (is_addr) {
			val = return_addr_of_var(result);
		} else {
			if (type_flag != 2)
				val = atoi(result);
			else {
				float f = atof(result);
				memcpy(&val, &f, sizeof(float));
			}
		}
		int param = val;
		generate_1(PUSHP, type_flag, is_addr, &param, 1);
		return;
	}

	// 处理 PUSH
	if (!strcmp(result, "push")) {
		s = read_a_token(s);
		s = skip_blank(s);
		char type;
		if (!strcmp(result, "B")) type = 0;
		else if (!strcmp(result, "I")) type = 1;
		else if (!strcmp(result, "F")) type = 2;
		else {
			printf("Error: PUSH missing type\n");
			exit(1);
		}
		s = read_a_token(s);
		s = skip_blank(s);
		int addr = return_addr_of_var(result);
		int params[1] = {addr};
		generate_4(PUSH, type, params, 1);
		return;
	}

	// 处理 BIT_AOX
if (!strcmp(result, "bit_aox")) {
    // 读取操作符
    s = read_a_token(s);
    s = skip_blank(s);
    int op_type = 0;
    if (!strcmp(result, "a")) op_type = 1;
    else if (!strcmp(result, "o")) op_type = 2;
    else if (!strcmp(result, "x")) op_type = 3;
    else {
        printf("Error: bit_aox requires A/O/X\n");
        exit(1);
    }
    // 读取 p1
    s = read_a_token(s);
    s = skip_blank(s);
    int p1_is_addr = (result[0] == '$');
    int p1_val;
    if (p1_is_addr) {
        p1_val = return_addr_of_var(result);
    } else {
        p1_val = atoi(result);
    }
    // 读取 p2
    s = read_a_token(s);
    s = skip_blank(s);
    int p2_is_addr = (result[0] == '$');
    int p2_val;
    if (p2_is_addr) {
        p2_val = return_addr_of_var(result);
    } else {
        p2_val = atoi(result);
    }
    // 确定 param_type: 0双立即数,1地址+立即数,2双地址
    char param_type;
    if (!p1_is_addr && !p2_is_addr) param_type = 0;
    else if (p1_is_addr && !p2_is_addr) param_type = 1;
    else if (p1_is_addr && p2_is_addr) param_type = 2;
    else {
        printf("Error: bit_aox does not support immediate first operand and address second operand\n");
        exit(1);
    }
    int params[3] = {op_type, p1_val, p2_val};
    generate_4(BIT_AOX, param_type, params, 3);
    return;
}

// 处理 BIT_MOV
if (!strcmp(result, "bit_move")) {
    // 读取方向 L/R
    s = read_a_token(s);
    s = skip_blank(s);
    int shift_dir = 0; // 0左移,1右移
    if (!strcmp(result, "l")) shift_dir = 0;
    else if (!strcmp(result, "r")) shift_dir = 1;
    else {
        printf("Error: bit_move requires L/R\n");
        exit(1);
    }
    // 读取 p1
    s = read_a_token(s);
    s = skip_blank(s);
    int p1_is_addr = (result[0] == '$');
    int p1_val;
    if (p1_is_addr) {
        p1_val = return_addr_of_var(result);
    } else {
        p1_val = atoi(result);
    }
    // 读取 p2
    s = read_a_token(s);
    s = skip_blank(s);
    int p2_is_addr = (result[0] == '$');
    int p2_val;
    if (p2_is_addr) {
        p2_val = return_addr_of_var(result);
    } else {
        p2_val = atoi(result);
    }
    // 确定 param_type 低2位
    char param_type;
    if (!p1_is_addr && !p2_is_addr) param_type = 0;
    else if (p1_is_addr && !p2_is_addr) param_type = 1;
    else if (p1_is_addr && p2_is_addr) param_type = 2;
    else {
        printf("Error: bit_move does not support immediate first operand and address second operand\n");
        exit(1);
    }
    // 将移位方向编码到 param_type 的 bit2
    param_type |= (shift_dir << 2);
    int params[2] = {p1_val, p2_val};
    generate_4(BIT_MOV, param_type, params, 2);
    return;
}
	
	// 处理 PUSH
	if (!strcmp(result, "timer")) {
		s = read_a_token(s);
		s = skip_blank(s);
		//这是为数不多不需要指定运算类型的指令之一（笑
		int addr = return_addr_of_var(result);
		int params[1] = {addr};
		generate_4(ARS_TIMER, 0, params, 1);
		return;
	}

	// 处理 CALL
	if (!strcmp(result, "call")) {
		s = read_a_token(s);
		s = skip_blank(s);
		// 查找函数地址
		uint32_t func_addr = 0;
		for (int i = 0; i < state.func_count; i++) {
			if (!strcmp(result, state.functions[i].name)) {
				func_addr = state.functions[i].addr;
				break;
			}
		}
		if (!func_addr) {
			printf("Error: undefined function %s\n", result);
			exit(1);
		}
		int params[1] = {func_addr};
		generate_3(CALL, params, 1);
		return;
	}

	if (!strcmp(result, "endfn") || !strcmp(result, "endmain")) {
		// 重置符号表
		symbol_count = 0;
		ttl_var_len = 0;
		level_count--;
		return;
	}

	// 处理 RET
	if (!strcmp(result, "ret")) {
		generate_3(RET, NULL, 0);
		return;
	}

	// 处理 HLT
	if (!strcmp(result, "hlt")) {
		// 重置符号表
		symbol_count = 0;
		ttl_var_len = 0;
		level_count--;
		generate_3(HLT, NULL, 0);
		return;
	}

	// 处理 JMP 和 JMP_T
	if (!strcmp(result, "jmp") || !strcmp(result, "jmp_t")) {
		int op = (!strcmp(result, "jmp")) ? JMP : JMP_T;
		s = read_a_token(s);
		s = skip_blank(s);
		// 记录未填充跳转
		strcpy(state.unlables[state.unlabel_count].name, result);
		state.unlables[state.unlabel_count].code_addr = (uint32_t)(ptr - codePool);
		state.unlables[state.unlabel_count].is_taken = 0;
		state.unlabel_count++;
		// 临时占位，后续回填
		int placeholder[1] = {0};
		generate_3(op, placeholder, 1);
		return;
	}

	// 其他指令暂未实现，忽略或报错
	printf("Warning: unimplemented instruction %s\n", result);
}

// 回填所有未解析的标签
void resolve_labels() {
	for (int i = 0; i < state.unlabel_count; i++) {
		// 查找标签地址
		uint32_t target = 0;
		for (int j = 0; j < state.label_count; j++) {
			if (!strcmp(state.unlables[i].name, state.labels[j].name)) {
				target = state.labels[j].addr;
				break;
			}
		}
		if (!target) {
			printf("Error: undefined label %s\n", state.unlables[i].name);
			exit(1);
		}
		// 回填到代码中
		uint32_t *addr = (uint32_t *)(codePool + state.unlables[i].code_addr + 1); // 跳过opcode
		*addr = target;
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: %s <source file>\n", argv[0]);
		return 1;
	}

	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		perror("Failed to open source file");
		return 1;
	}

	char line[1024],*x;
	int line_num = 0;
	while (fgets(line, sizeof(line), fp)) {
		line_num++;
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
		x = skip_blank(line);
		compile_one_line(x);
	}
	fclose(fp);

	// 回填跳转标签
	resolve_labels();

	// 输出二进制文件
	char out_file[512];
	strcpy(out_file, argv[1]);
	char *dot = strrchr(out_file, '.');
	if (dot) strcpy(dot, ".ars_bin");
	else strcat(out_file, ".ars_bin");

	FILE *out = fopen(out_file, "wb");
	if (!out) {
		perror("Failed to create output file");
		return 1;
	}
	size_t code_size = ptr - codePool;
	fwrite(codePool, 1, code_size, out);
	fclose(out);

	printf("\nCompiled successfully to %s (%zu bytes)\n", out_file, code_size);
	printf("Bytecode dump:{\n");
	for (size_t i = 0; i < code_size; i++) {
		printf("0x%02X,", codePool[i]);
		if ((i + 1) % 16 == 0) printf("\n");
	}
	printf("}\n");
	return 0;
}
