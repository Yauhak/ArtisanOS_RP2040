#include "INTERPRETER.h"
#include "Memory.h"
#include "ByteCode.h"

extern ars_i32 CalcResu[OS_MAX_TASK];
extern int needJump[OS_MAX_TASK];
extern volatile uars_i8 *CurCmd[OS_MAX_TASK];

const char paramQ[] = { 2, 2, 2, 2, 1,
                        1, 2, 2, 2, 2,
                        2, 2, 2, 2, 2,
                        2, 1, 1, 1, 0,
                        2, 3, 1, 2, 2,
                        1, 1, 1, 2, 1,
                        0 };


void setup() {
  //Serial.begin(115200);
  delay(1000);
  //Serial.println("RP2040 ARS VM Starting...");
  init_mem_info();
  ARS_memset(OS_EXE_LOAD_START(0), Fibonacci, Fibonacci_Len);
  ARS_memset(OS_EXE_LOAD_START(1), LED_Flash, LED_Flash_Len);
  //Serial.println("Load Programs successed!");
  call(0, (ars_i32 *)OS_EXE_LOAD_START(0), 0);
  call(0, (ars_i32 *)OS_EXE_LOAD_START(1), 1);
  needJump[0] = 0;
  needJump[1] = 0;
}

void loop() {
  // 非阻塞指令调度
  static uars_i8 tid = 0;
  if (CurCmd[tid]) {
    uars_i8 ins = *CurCmd[tid]++;
    ars_i32 params[3];
    ARS_memset(params, (const void *)CurCmd[tid], 4 * paramQ[ins >> 3]);
    if ((ins >> 3) == CALL) CurCmd[tid] += 4;
    interprete(ins, params, tid);
    if (!needJump[tid] || ((ins >> 3) == JMP_T && !CalcResu[tid]))
      CurCmd[tid] += sizeof(int) * paramQ[ins >> 3];
    needJump[tid] = 0;
  }
  tid = (tid + 1) % OS_MAX_TASK;
  //让出 CPU 以便其他 Arduino 任务（如串口处理）运行
  //实际生产中可去掉该delay
  delay(1);
}