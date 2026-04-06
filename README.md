# ArtisanOS_RP2040

An experimental embedded operating system and bytecode virtual machine for Arduino RP2040 (Raspberry Pi Pico).

---

## 项目简介 | Project Overview

**ArtisanOS_RP2040** 是一个运行在树莓派 Pico（RP2040）上的极简“操作系统”，内置自研的字节码虚拟机、动态内存模型、多进程任务调度，以及一种脑洞大开的“ARS伪汇编”编程语言和通用编译器。

ArtisanOS_RP2040 is a minimal "operating system" for the Raspberry Pi Pico (RP2040), featuring a custom bytecode VM, dynamic memory model, multiprocess task scheduling, and a wildly creative "ARS pseudo-assembly" language and general-purpose compiler.

---

## 主要特性 | Key Features

- **ARS 虚拟机**：支持多任务切换的自研字节码虚拟机，可以加载和运行任意“ARS字节码”程序
- **动态内存与任务调度**：支持内存碎片整理，任务上下文保存，虚拟“进程”
- **ARS 伪汇编语言**：极具个人风格的编译型语言，既像汇编又像高级语言，能直接描述变量、函数、标签和各种运算
- **高可扩展性**：便于开发自己的库/操作系统功能或新指令
- **可移植性**：理论上任何32位系统都可复刻（只要有C++和少量C库即可）
- **Arduino集成**：开箱即用，直接在Pico上加载

---

## 快速上手 | Quick Start

1. **Clone & Build**

    ```bash
    git clone https://github.com/Yauhak/ArtisanOS_RP2040.git
    cd ArtisanOS_RP2040
    # See docs for Arduino/build setup
    ```

2. **编译 ARS 伪汇编到字节码 | Compile ARS Code:**

    ```bash
    gcc Compiler/Source/Compiler.c -o arscc
    ./arscc Compiler/Demo/LEDFlash.txt
    ```

    输出将生成 `.ars_bin` 文件，字节码可用于虚拟机直接执行。

3. **烧录/运行 & 任务调度 | Flash & Run**  
   用 Arduino IDE 上传，Pico 上电后将自动运行内置的 LED_Flash 与 LED_Stream 程序，进行定时与流水灯演示。  

---

## ARS 伪汇编语言奇特语法介绍  
### ARS Pseudo-Assembly Language: Syntax & Examples

#### 1. 结构范例 | Example Structure

```
fn add_func         // 定义子程序
  mem
    $a I 0         // 变量声明：$a, 整型, 初值0
    $b I 0
  end_mem
  add I $a $b      // 运算：int型 $a + $b, 结果→CalcResu
  ret
endfn

main
  mem
    $x I 0
    $y I 0
  end_mem
  mov I $x 5
  mov I $y 7
  pushp I $x
  pushp I $y
  call add_func    // 调用子程序
  hlt              // 程序结束
endmain
```

#### 2. 基本指令形式 | Basic Instruction Format

- `mov I $x 10`：将整数10存入变量`$x`
- `add I $a $b`：将`$a+$b`存入计算寄存器
- `push I $res`：保存计算结果到`$res`
- `call func`：调用函数
- `hlt`：程序停止

#### 3. 数据类型 | Data Types

- `B` 单字节(byte)  
- `I` 四字节整型(int)  
- `F` 四字节浮点(float) 

#### 4. 变量声明 | Variable Declaration

在`mem ... end_mem`内声明，可带初值，不写默认0：
```
$a I 0      // 整型变量
$buf B 0
$pi F 3.14
```

#### 5. 条件与跳转 | Condition & Branch

- `eq I $a $b`：判断$a==$b（结果存寄存器）
- `jmp_t label`：若“结果寄存器”非0则跳转到label
- `jmp label`：无条件跳转
- `lb label`：定义标签

#### 6. 子程序与参数传递 | Functions & Params

- `pushp I 3`：传递参数3(int)
- `pushp I $x`：传递变量地址
- `call func`：调用
- `ret`：返回

#### 7. GPIO/定时器操作 | GPIO/Timer Operation

- `gpio_write $pio 1`：将引脚写高
- `gpio_write 18 0`：将18号引脚写低
- `timer $now`：读取当前系统计时
- `ars_timer` 等价于`timer`指令

#### 8. 内存数组 | Array

初始化、访问数组范例：
```
mem
  $arr I 0
end_mem
init_array I $arr 2 5 6 7     // 初始化3个元素
set_array I $arr 1 88         // $arr[1]=88
read_array I $arr 1 $t        // $t = $arr[1]
```

---

## 示例 | Example

**LED 闪烁程序 | LED Flash Program**

```
main
  mem
    $pio I 0
  end_mem
  mov I $pio 6
lb loop_main
  gpio_write $pio 1
  pushp I 100 
  call delay_ms
  gpio_write $pio 0
  pushp I 100 
  call delay_ms
jmp loop_main
hlt
endmain
```

**流水灯 | LED Stream**

```
main
  mem
    $delay I 0
  end_mem
  mov I $delay 100

lb loop_main
  gpio_write 18 1
  pushp I $delay
  call delay_ms
  gpio_write 18 0

  gpio_write 19 1
  pushp I $delay
  call delay_ms
  gpio_write 19 0

  gpio_write 20 1
  pushp I $delay
  call delay_ms
  gpio_write 20 0

jmp loop_main
hlt
endmain
```

---

## 原创字节码结构 | Custom Bytecode Structure

- 每个指令对应唯一1字节操作码，一条操作指令后跟随参数
- For details, see `Core/Compiler/Source/Compiler.c` and `Core/INTERPRETER.cpp`.

---

## 参考 Documentation

- [`Compiler/Demo/`](Compiler/Demo/) 目录下有丰富样例
- [Compiler/Source/Compiler.c](Compiler/Source/Compiler.c)
- [Core/INTERPRETER.h](Core/INTERPRETER.h)

---

## Contributors

欢迎 Issues/PRs/星星讨论~  
Feel free to open issues, PRs, or discussions!

---

## License

MIT License

---

## 致谢 | Thanks

本项目代码与灵感均出自 @Yauhak，特别鸣谢所有关注极客 DIY 嵌入式开发的同好。

---

# ARS 语言：它很奇特

“ARS伪汇编” = 汇编的自由 + 高级语言结构 + 最小实现。  
如果你觉得脑洞很大，请加入我们一起扩展这门神奇的“自定义系统编程语言”！

---
