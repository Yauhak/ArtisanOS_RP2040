# ArtisanOS_RP2040

An experimental embedded operating system and bytecode virtual machine for Arduino RP2040 (Raspberry Pi Pico).

---

## 项目简介 | Project Overview

**ArtisanOS_RP2040** 是一个极简“操作系统”，为树莓派Pico（RP2040）量身打造，核心包含独立实现的字节码虚拟机、动态内存管理、任务调度机制，以及一门自创的奇特“ARS伪汇编”语言（配套通用编译器）。

ArtisanOS_RP2040 is a minimal OS for the RP2040, featuring a custom bytecode virtual machine, dynamic memory with fragmentation handling, task switching, and the highly original "ARS pseudo-assembly" language and compiler.

---

## 主要特性 | Key Features

- **自定义ARS字节码虚拟机**  
  Custom bytecode VM
- **多任务调度与动态内存管理**  
  Multi-task scheduling & dynamic memory management (with fragmentation merge)
- **脑洞大开的ARS伪汇编语言**  
  Wildly creative "ARS pseudo-assembly"
- **极小依赖，极易移植**  
  Minimal dependencies - highly portable
- **适配Arduino Pico平台**  
  Fits within Arduino Pico resource constraints

---

## 项目结构 | Project Structure

```
ArtisanOS_RP2040/
├── Core/
│   ├── main.ino          # Arduino入口
│   ├── INTERPRETER.*     # 解释器&虚拟机实现
│   ├── Memory.*          # 虚拟机内存管理
│   ├── ByteCode.h        # 字节码/示例程序
├── Compiler/
│   ├── Source/Compiler.* # 字节码编译器
│   └── Demo/             # 伪高级语言Demo
└── README.md             # 项目说明
```

---

## 快速上手 | Quick Start

1. **Clone & Build**

    ```bash
    git clone https://github.com/Yauhak/ArtisanOS_RP2040.git
    cd ArtisanOS_RP2040
    # 参见文档了解Arduino环境构建
    ```

2. **编译你的ARS程序 | Compile your ARS code**

    ```bash
    gcc Compiler/Source/Compiler.c -o arscc
    ./arscc Compiler/Demo/LEDFlash.txt
    ```

    输出将生成 `.ars_bin` 文件，直接可被加载至虚拟机运行。

3. **烧录/执行 | Flash & Run**

    用 Arduino IDE 上传，Pico 上电后自动运行内置字节码程序。

---

## ARS 语言奇特语法说明  
### ARS Pseudo-Assembly Language: Peculiarities & Guide

#### 1. 变量声明与数组 | Variable & Array Declaration

**声明语法：**  
```
$变量名 类型 长度
```
- **长度不是初值！**长度含义为“分配(长度+1)个元素”。即：
    - `$x I 0` —— `int` 类型单变量，占4字节（1个元素）。
    - `$arr B 3` —— `byte`类型，长度4数组（`3+1=4`）。
    - `$y F 2` —— `float`类型，长度3数组。

> ⚠️ **注意/Note:**  
> `长度`字段为“实际申请元素数量 - 1”，存储空间实际为`(长度+1) * 类型大小`。这是ARS语言的独有设计。  

举例/Examples:
```
mem
  $flag B 0      ; 单字节变量
  $vec I 2       ; 3元素int数组（2+1=3）
end_mem
```

#### 2. set_array / read_array 指令

- 用法: `set_array 类型 数组名 下标`
- 用法: `read_array 类型 数组名 下标`
- 说明：**只有三个参数！**  
  操作的值（被写入/读取）在特殊寄存器 `CalcResu` 里。
    - `set_array` ：将 `CalcResu` 的值写入指定数组元素。
    - `read_array` ：将指定数组元素读入 `CalcResu`。

- 用例 / Example:
    ```
    set_array I $vec 1     ; $vec[1] = CalcResu
    read_array I $vec 2    ; CalcResu = $vec[2]
    ```

#### 3. init_array 数组初始化

```
init_array 类型 数组名 元素数量 [元素1] [元素2] ...
```
- `元素数量`同样是“实际元素数-1”。比如写2代表初始化3个元素。
- 每个元素可以为字面值或地址。
- 支持变长参数。

> ⚠️ **注意/CAUTION:**  
> `元素数量`实为“你要填入的元素数-1”，比如想要初始化4个元素，应写3。

**示例：**
```
init_array I $vec 2 5 6 7   ; 实际为$vec有3个元素：5,6,7
```

#### 4. 条件跳转与寄存器 | Condition, Result Register

- 条件判断指令如`eq`、`lt`、`ge`等会把结果写入`CalcResu`寄存器
- `jmp_t`根据`CalcResu!=0`来决定是否跳转

---

### 综述 | Language Structure Summary

#### 变量声明 | Variable Definition

```
; 定义单变量
$a I 0            ; int
; 定义3元素数组
$array F 2        ; float[3]
```

#### 流程/语法 | Control & Syntax:

```
main
  mem
    $x I 0
    $y I 2   ; $y[3]
  end_mem
  mov I $x 10
  eq I $x 10
  jmp_t is_ten
  hlt
lb is_ten
  ; ...
endmain
```

---

## 示例 | Example

**声明数组&读写**  
```
main
  mem
    $buf I 2      ; $buf[3]
    $tmp I 0
  end_mem
  ; 初始化3个元素值为5,6,7
  init_array I $buf 2 5 6 7
  ; 读取buf[2]
  read_array I $buf 2     ; CalcResu = $buf[2]
  ; 写buf[1]
  set_array I $buf 1      ; $buf[1] = CalcResu
hlt
endmain
```

---

## 关键操作指令参考 | Key ARS Instructions Reference


| 指令        | 语法                                                                 | 作用                                                                 | 示例                                                                 |
|-------------|----------------------------------------------------------------------|----------------------------------------------------------------------|----------------------------------------------------------------------|
| MOV         | mov 类型 目标 源；类型：B(字节), I(整型), F(浮点)；源可为立即数或地址（地址前加$） | 将源数据存入目标地址                                                   | mov I $x 10；mov B $flag $temp                                       |
| EXT_BYTE    | ext_byte 目标 源；目标：I型地址，源：B型地址                           | 将源字节扩展为32位整型，存入目标地址                                     | ext_byte $intVar $byteVar                                            |
| SETARRAY    | set_array 类型 数组名 下标；下标可为立即数或地址（加$）                  | 将 CalcResu 的值写入数组的指定元素                                      | set_array I $vec 2                                                   |
| READARRAY   | read_array 类型 数组名 下标                                           | 将数组指定元素的值读入 CalcResu                                        | read_array I $vec $idx                                               |
| INITARRAY   | init_array 类型 数组名 元素数 [值列表]；元素数 = 实际元素个数 - 1；值可为立即数或地址 | 批量初始化数组元素                                                     | init_array I $vec 2 5 6 7                                            |
| PUSH        | push 类型 地址                                                        | 将 CalcResu 的值存入指定地址                                           | push I $result                                                       |
| PUSHP       | pushp 类型 值/地址                                                    | 将参数压入子程序参数栈（供 CALL 使用）                                   | pushp I 100；pushp F $pi                                             |
| ADD         | add 类型 操作数1 操作数2；类型：I(整型) 或 F(浮点)                      | 加法，结果存入 CalcResu                                                | add I $a $b；add F $x 3.14                                           |
| SUB         | sub 类型 操作数1 操作数2                                              | 减法，结果存入 CalcResu                                                | sub I $total $cost                                                   |
| MUL         | mul 类型 操作数1 操作数2                                              | 乘法，结果存入 CalcResu                                                | mul I $width $height                                                 |
| DIV         | div 类型 操作数1 操作数2                                              | 除法，结果存入 CalcResu（整数除向零舍入）                                | div F $total $count                                                  |
| EQ          | eq 类型 操作数1 操作数2                                               | 相等比较，结果（1=真，0=假）存入 CalcResu                               | eq I $x 10                                                           |
| LT          | lt 类型 操作数1 操作数2                                               | 小于比较                                                              | lt I $age 18                                                         |
| GT          | gt 类型 操作数1 操作数2                                               | 大于比较                                                              | gt F $temp 36.5                                                      |
| LE          | le 类型 操作数1 操作数2                                               | 小于等于比较                                                          | le I $index $max                                                     |
| GE          | ge 类型 操作数1 操作数2                                               | 大于等于比较                                                          | ge I $score 60                                                       |
| NE          | ne 类型 操作数1 操作数2                                               | 不等于比较                                                            | ne F $flag 0                                                         |
| JMP         | jmp 标签                                                             | 无条件跳转到指定标签                                                   | jmp loop_start                                                       |
| JMP_T       | jmp_t 标签                                                           | 若 CalcResu 不为 0 则跳转到标签                                        | jmp_t is_equal                                                       |
| CALL        | call 子程序名                                                        | 调用子程序（子程序需先用 fn … endfn 定义）                              | call delay_ms                                                        |
| RET         | ret                                                                 | 从子程序返回调用点                                                     | ret                                                                  |
| BIT_AOX     | bit_aox 操作 操作数1 操作数2；操作：A(AND), O(OR), X(XOR)              | 按位与/或/异或，结果存入 CalcResu                                      | bit_aox A $mask $flags                                               |
| BIT_MOV     | bit_move 方向 操作数 位数；方向：L(左移) 或 R(右移)；位数可为立即数或地址 | 按位移位，结果存入 CalcResu                                            | bit_move L $value 2；bit_move R $value $bits                  |
| ARS_TIMER   | timer 地址                                                           | 将系统毫秒数（millis()）存入指定地址（整型）                             | timer $start_time                                                    |
| GPIO_WRITE  | gpio_write 引脚 值；引脚/值可为立即数或地址                             | 设置指定 GPIO 引脚输出高低电平（值：0/1）                               | gpio_write 13 1；gpio_write $pin $val                                |
| GPIO_READ   | gpio_read 目标地址 引脚；引脚可为立即数或地址                           | 读取指定 GPIO 引脚电平，存入目标地址                                    | gpio_read $state 18；gpio_read $value $pin                           |
| VAL         | val 类型 值/地址；类型：B/I/F                                         | 将立即数或内存中的值载入 CalcResu                                      | val I 100；val F $pi                                                 |
| TO_INT      | to_int 地址；地址处必须为浮点数                                        | 将地址处的浮点数转换为整型（截断），存入 CalcResu                        | to_int $floatVar                                                     |
| TO_FLOAT    | to_float 地址；地址处必须为整型                                        | 将地址处的整型转换为浮点数，存入 CalcResu（位模式不变）                  | to_float $intVar                                                     |
| HLT         | hlt                                                                 | 终止当前程序，释放其占用的内存和任务槽                                   | hlt                                                                  |
> 更多样例请见 [Compiler/Demo/](Compiler/Demo/)。

---

## License

MIT License

---

## 致谢 | Thanks

本项目由[Yauhak](https://github.com/Yauhak)原创开发，非常欢迎各种建议、star、PR与讨论！

---

# ARS 语言：设计的奇特性说明

- 变量/数组声明时的“长度”原则，与C等传统语言不同，**需手动+1**。
- `set_array`、`read_array`始终只用3参数，均通过`CalcResu`寄存器读写。
- `init_array`的元素数与实际初始化数目存在(+1)的特立独行的差异，请仔细留意。
- 所有运算/比较指令都不支持对单字节数进行操作，请先使用ext_byte命令将其扩展为4字节整型
- 可以通过mov指令将4字节整型转换成单字节
- 灵活组合低级与高级特性，极适合极客DIY/嵌入式系统学习与实验。

---

**如果你喜欢这种独特的开发风格，请参与一起完善和扩展这个“属于自己的微型操作系统世界”！**

---
