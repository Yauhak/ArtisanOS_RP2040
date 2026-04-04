#include "INTERPRETER.h"
#include "Memory.h"
#include "ByteCode.h"
#include <Arduino.h>

extern int32_t CalcResu[OS_MAX_TASK];
extern int needJump[OS_MAX_TASK];
extern volatile uint8_t *CurCmd[OS_MAX_TASK];

const char paramQ[] = { 2, 3, 1, 1, 2,
                        2, 2, 2, 2, 2,
                        2, 2, 2, 2, 1,
                        1, 1, 0, 2, 2,
                        1, 2, 2, 0 };

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("RP2040 ARS VM Starting...");
  init_mem_info();
  memcpy(OS_EXE_LOAD_START(0), LED_Breath, LED_Breath_Len);
  memcpy(OS_EXE_LOAD_START(1), LED_Stream, LED_Stream_Len);
  Serial.println("Load Programs successed!");
  call(0, (int32_t *)OS_EXE_LOAD_START(0), 0);
  call(0, (int32_t *)OS_EXE_LOAD_START(1), 1);
  needJump[0] = 0;
  needJump[1] = 0;
}

void loop() {
  // 非阻塞指令调度
  static uint8_t tid = 0;
  if (CurCmd[tid]) {
    uint8_t ins = *CurCmd[tid]++;
    int32_t params[3];
    memcpy(params, (const void *)CurCmd[tid], 4 * paramQ[ins >> 3]);
    if ((ins >> 3) == CALL) CurCmd[tid] += 4;
    interprete(ins, params, tid);
    if (!needJump[tid] || ((ins >> 3) == JMP_T && !CalcResu[tid]))
      CurCmd[tid] += sizeof(int) * paramQ[ins >> 3];
    needJump[tid] = 0;
  }
  tid = (tid + 1) % OS_MAX_TASK;
  // 让出 CPU 以便其他 Arduino 任务（如串口处理）运行
  delay(1);
}