#ifndef IO_INCLUDE
	#include "IO_INCLUDE.h"
#endif

volatile void *ARS_memmove(volatile void *dest, volatile const void *src, int n) {
	if (dest == (void * )0 || src == (void * )0 || n <= 0) return dest;
	volatile char *d = (volatile char *)dest;
	volatile const char *s = (volatile const char *)src;
	if (d < s) {
		for (int i = 0; i < n; i++) d[i] = s[i];
	} else {
		for (int i = n - 1; i >= 0; i--) d[i] = s[i];
	}
	return dest;
}

volatile void *ARS_memset(volatile void *dest, volatile const void *byte, int n) {
	if (dest == 0 || byte == 0 || n == 0) {
		return dest; // 处理无效参数
	}
	volatile char *d = (volatile char *)dest;
	volatile const char *s = (volatile const char *)byte;
	for (int i = 0; i < n; i++) {
		*d++ = *s++;
	}
	return dest;
}

uars_i16 ARS_strlen(const char *str) {
	const char *ptr = str;
	uars_i16 len = 0;
	while (*ptr++ != 0)len++;
	return len;
}

uars_i8 ARS_strcmp(const char *haystack, const char *needle, int len) {
	if (*needle == '\0') {
		return 1;
	}
	for (int i = 0; i < len; i++) {
		if (haystack[i] != needle[i]) {
			return 1;
		}
	}
	return 0;
}

uars_i32 ARS_strtok(char *str, const char delim) {
	char *ptr = str;
	while (*ptr != delim && ptr < str + ARS_strlen(str))ptr++;
	if (*ptr == delim) {
		return ptr - str;
	}
	return 0;
}

//这两个函数将相同的内存块进行原封不动的抄写，并以不同形式（INT,FLOAT）读出来
//而内存块的内容并没有改变
//像float x=1.14;int y=x;就会改变因为类型转换而造成内存内容的不同
float copyIntToFloat(int x) {
	float result;
	ARS_memmove(&result, &x, sizeof(int));  // 安全转换
	return result;
}

int copyFloatToInt(float x) {
	int result;
	ARS_memmove(&result, &x, sizeof(float));  // 安全转换
	return result;
}
