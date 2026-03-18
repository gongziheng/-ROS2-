# GDB 系统学习手册（含习题）

> 适合 Linux / C / C++ / ROS 2 初学者到进阶开发者。
>
> 学习目标：从“会下断点”提升到“能独立定位崩溃、逻辑错误、死循环、线程问题”。

---

## 目录

1. [GDB 是什么](#1-gdb-是什么)
2. [学习前准备](#2-学习前准备)
3. [第一个最小示例](#3-第一个最小示例)
4. [GDB 最核心的调试流程](#4-gdb-最核心的调试流程)
5. [常用命令速查表](#5-常用命令速查表)
6. [断点：调试的入口](#6-断点调试的入口)
7. [单步执行：next / step / finish](#7-单步执行next--step--finish)
8. [查看变量、表达式和类型](#8-查看变量表达式和类型)
9. [调用栈与栈帧](#9-调用栈与栈帧)
10. [观察点 watch：盯住变量变化](#10-观察点-watch盯住变量变化)
11. [内存查看与修改](#11-内存查看与修改)
12. [调试崩溃程序](#12-调试崩溃程序)
13. [core dump 分析](#13-core-dump-分析)
14. [attach 到正在运行的进程](#14-attach-到正在运行的进程)
15. [多线程程序调试](#15-多线程程序调试)
16. [C++ 调试重点](#16-c-调试重点)
17. [汇编视角和寄存器基础](#17-汇编视角和寄存器基础)
18. [共享库与源码不匹配的常见问题](#18-共享库与源码不匹配的常见问题)
19. [ROS 2 / Linux 开发中的常见用法](#19-ros-2--linux-开发中的常见用法)
20. [推荐学习路线](#20-推荐学习路线)
21. [练习题](#21-练习题)
22. [练习题参考答案](#22-练习题参考答案)
23. [最终速查清单](#23-最终速查清单)

---

## 1. GDB 是什么

GDB（GNU Debugger）是 GNU 提供的命令行调试器，主要用来调试 C、C++ 等程序。

你可以把它理解成：

**让程序在你想看的位置停下来，然后把运行现场展开给你看。**

GDB 最常见的作用有：

- 启动程序并控制执行过程
- 在某一行或某个函数停住
- 单步执行代码
- 查看变量值和表达式
- 分析函数调用链
- 定位段错误（Segmentation fault）
- 调试 core 文件
- 调试多线程程序
- 附加到已运行进程分析卡住问题

---

## 2. 学习前准备

### 2.1 安装 GDB

在 Ubuntu / Debian 上：

```bash
sudo apt update
sudo apt install gdb
```

查看版本：

```bash
gdb --version
```

### 2.2 编译时必须带调试信息

GDB 要想看到源码、变量名、函数名，编译时必须加 `-g`。

推荐开发期这样编译：

```bash
g++ -g -O0 -Wall -Wextra main.cpp -o app
```

参数说明：

- `-g`：生成调试信息
- `-O0`：关闭优化，避免变量被优化掉
- `-Wall -Wextra`：提前暴露更多潜在问题

### 2.3 为什么不建议调试时开优化

如果你用：

```bash
g++ -g -O2 main.cpp -o app
```

常见现象：

- 变量显示 `<optimized out>`
- 单步执行时“跳来跳去”
- 某些源码行和执行位置对不上

初学阶段尽量使用 `-O0`。

---

## 3. 第一个最小示例

先用这个简单程序练最基本命令：

```cpp
#include <iostream>
#include <vector>

int add(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    std::vector<int> nums = {1, 2, 3};
    int sum = 0;

    for (int n : nums) {
        sum = add(sum, n);
    }

    std::cout << "sum = " << sum << std::endl;
    return 0;
}
```

编译：

```bash
g++ -g -O0 main.cpp -o app
```

启动 GDB：

```bash
gdb ./app
```

---

## 4. GDB 最核心的调试流程

调试时最常见的流程其实很固定：

1. 进入 GDB
2. 设置断点
3. 运行程序
4. 程序停住后查看变量
5. 决定是单步还是继续运行
6. 必要时查看调用栈
7. 找到问题位置和错误传播路径

最常见的一组命令就是：

```gdb
b main
r
n
s
p 变量名
bt
c
q
```

你只要真正熟练这一组，已经能解决很多日常问题。

---

## 5. 常用命令速查表

| 命令 | 含义 |
|---|---|
| `run` / `r` | 运行程序 |
| `break` / `b` | 设置断点 |
| `next` / `n` | 单步执行，不进入函数 |
| `step` / `s` | 单步执行，进入函数 |
| `finish` | 运行到当前函数返回 |
| `continue` / `c` | 继续运行 |
| `print` / `p` | 打印变量或表达式 |
| `ptype` | 查看类型 |
| `list` / `l` | 查看源码 |
| `bt` | 查看调用栈 |
| `frame` | 查看或切换栈帧 |
| `info locals` | 查看局部变量 |
| `info args` | 查看函数参数 |
| `watch` | 设置观察点 |
| `info breakpoints` | 查看断点 |
| `delete` | 删除断点 |
| `x` | 查看内存 |
| `set var` | 修改变量值 |
| `info threads` | 查看线程 |
| `thread` | 切换线程 |
| `disassemble` | 反汇编 |
| `quit` / `q` | 退出 |

---

## 6. 断点：调试的入口

### 6.1 按函数名打断点

```gdb
b main
b add
```

这是最常用、最稳妥的方式。

### 6.2 按行号打断点

```gdb
b 12
```

前提是当前源码文件明确，且第 12 行有可执行代码。

### 6.3 按文件:行号打断点

```gdb
b main.cpp:12
```

多文件项目里更推荐这种写法。

### 6.4 查看断点列表

```gdb
info breakpoints
```

简写：

```gdb
i b
```

### 6.5 删除断点

删除指定断点：

```gdb
delete 1
```

删除所有断点：

```gdb
delete
```

### 6.6 禁用 / 启用断点

```gdb
disable 1
enable 1
```

### 6.7 条件断点

只有条件满足时才停：

```gdb
b main.cpp:15 if sum == 3
b add if a == 1
```

这在循环里特别有用，不然程序会频繁停下。

### 6.8 临时断点

命中一次后自动删除：

```gdb
tbreak main
```

---

## 7. 单步执行：next / step / finish

### 7.1 next

```gdb
n
```

作用：执行当前行，如果当前行调用了函数，不进入函数内部。

适合场景：

- 你只想看当前函数逻辑
- 被调函数你暂时不关心

### 7.2 step

```gdb
s
```

作用：执行当前行，如果当前行调用了函数，会进入函数内部。

适合场景：

- 你怀疑 bug 在被调用函数里
- 你需要跟踪参数是如何被处理的

### 7.3 finish

```gdb
finish
```

作用：执行到当前函数返回。

适合场景：

- 不想在函数内部一步一步跟
- 想直接看到当前函数返回值

### 7.4 until

```gdb
until
```

作用：运行到当前代码块后面的位置，常用于跳过一段循环。

### 7.5 continue

```gdb
c
```

作用：继续执行，直到下一个断点或程序结束。

---

## 8. 查看变量、表达式和类型

### 8.1 打印变量

```gdb
p sum
p result
```

### 8.2 打印表达式

```gdb
p sum + 10
p nums[0]
p ptr->value
```

### 8.3 查看局部变量

```gdb
info locals
```

### 8.4 查看函数参数

```gdb
info args
```

### 8.5 查看类型

```gdb
ptype sum
ptype nums
ptype obj
```

### 8.6 自动显示变量

```gdb
display sum
```

以后每次程序停下，GDB 都会自动显示 `sum`。

查看自动显示列表：

```gdb
info display
```

取消：

```gdb
undisplay 1
```

### 8.7 修改变量值

```gdb
set var sum = 100
set var flag = true
```

这非常适合做“假设验证”：

- 如果把这个值改掉，程序还会不会崩？
- 如果强行跳过某个条件，流程会怎么走？

---

## 9. 调用栈与栈帧

当程序崩溃或者逻辑很复杂时，调用栈非常重要。

### 9.1 查看调用栈

```gdb
bt
```

输出会告诉你：

- 当前停在哪个函数
- 这个函数是谁调用的
- 再上一层调用者是谁

### 9.2 查看更完整的调用栈

```gdb
bt full
```

会额外显示局部变量信息。

### 9.3 切换栈帧

向上切：

```gdb
up
```

向下切：

```gdb
down
```

或者直接切到指定帧：

```gdb
frame 0
frame 1
frame 2
```

### 9.4 为什么调用栈重要

很多时候崩溃点只是“最后倒下的地方”，真正问题可能更早。

例如：

- 某函数收到空指针才崩
- 但空指针是上层函数传错的
- 继续 `up` 才能找到真正责任点

所以调试崩溃时不要只盯着 `frame 0`。

---

## 10. 观察点 watch：盯住变量变化

断点是“走到某个位置停”，观察点是“某个值一变就停”。

### 10.1 监控写入

```gdb
watch sum
```

当 `sum` 被修改时，程序会停住。

### 10.2 监控读取

```gdb
rwatch sum
```

当 `sum` 被读取时停住。

### 10.3 监控读写

```gdb
awatch sum
```

### 10.4 观察点的典型应用

适合查这类问题：

- 某个变量什么时候被改坏了
- 某个指针什么时候变成空了
- 状态机变量什么时候进入错误状态

---

## 11. 内存查看与修改

### 11.1 `x` 命令基本格式

```gdb
x/数量格式单位 地址
```

例如：

```gdb
x/4xw &sum
```

含义：

- `4`：显示 4 个单位
- `x`：十六进制
- `w`：每个单位 4 字节

### 11.2 常见格式

- `x`：十六进制
- `d`：十进制
- `u`：无符号十进制
- `c`：字符
- `s`：字符串
- `i`：指令

例子：

```gdb
x/s ptr
x/10d arr
x/8xb buffer
x/5i $pc
```

### 11.3 查看寄存器

```gdb
info registers
```

查看程序计数器附近指令：

```gdb
x/10i $pc
```

### 11.4 修改内存

一般初学阶段不建议直接改原始内存，更常用的是改变量：

```gdb
set var a = 10
```

---

## 12. 调试崩溃程序

最常见的崩溃包括：

- 空指针解引用
- 数组越界
- 野指针
- 重复释放内存
- 除零

### 12.1 典型调试流程

```gdb
gdb ./app
(gdb) run
```

程序崩溃后：

```gdb
bt
frame 0
list
info locals
info args
```

然后继续：

```gdb
up
info locals
```

### 12.2 崩溃排查时的思路

不要只问：

> 它为什么在这里崩？

更要问：

> 它为什么会带着这种错误输入走到这里？

也就是：

1. 先定位崩溃点
2. 再查错误值从哪传来的
3. 再找最早的异常点

---

## 13. core dump 分析

有时候程序崩溃发生在现场机、测试机、机器人上，你不能重现，只能分析 core 文件。

### 13.1 开启 core dump

```bash
ulimit -c unlimited
```

### 13.2 程序崩溃后分析 core

```bash
gdb ./app core
```

### 13.3 进入后常用命令

```gdb
bt
bt full
frame 0
info locals
info args
list
```

### 13.4 core 分析的关键点

要保证三者匹配：

- 可执行文件 `app`
- core 文件
- 对应源码和调试符号

如果二进制和源码不匹配，你看到的结果可能是错的。

### 13.5 常见问题

#### 看不到源码

通常是没带 `-g` 编译。

#### 栈很乱

可能是：

- 栈被破坏
- 二进制和 core 不匹配
- 优化过高

---

## 14. attach 到正在运行的进程

当程序不是“崩了”，而是“卡住了、CPU 很高、逻辑异常”，可以 attach 到运行中的进程。

### 14.1 找进程号

```bash
ps -ef | grep app
```

或者：

```bash
pidof app
```

### 14.2 attach

```bash
gdb -p 12345
```

### 14.3 attach 后常用操作

```gdb
bt
info threads
thread apply all bt
```

### 14.4 继续运行或脱离

继续运行：

```gdb
c
```

脱离进程但不结束它：

```gdb
detach
```

退出 GDB：

```gdb
q
```

### 14.5 典型场景

- 程序卡住不退出
- 某线程疑似死锁
- 某节点 CPU 占用异常
- 线上服务不能重启，只能现场分析

---

## 15. 多线程程序调试

线程一多，问题就会复杂很多。

### 15.1 查看线程列表

```gdb
info threads
```

### 15.2 切换线程

```gdb
thread 2
```

### 15.3 查看所有线程调用栈

```gdb
thread apply all bt
```

这是排查死锁和线程卡住时最重要的命令之一。

### 15.4 常见多线程调试思路

#### 场景 1：程序卡住

先执行：

```gdb
thread apply all bt
```

看：

- 哪个线程在等锁
- 哪个线程持有锁
- 哪个线程在死循环

#### 场景 2：某共享变量被改坏

可以对变量下 `watch`，但多线程下要注意命中频率可能很高。

#### 场景 3：线程崩溃

切到崩溃线程看：

```gdb
thread 3
bt
frame 0
```

---

## 16. C++ 调试重点

### 16.1 打印对象与成员

```gdb
p obj
p obj.member
p ptr->member
```

### 16.2 查看 STL 容器

现代 GDB 通常可以较好显示 STL：

```gdb
p vec
p vec.size()
p vec[0]
p str
p mp
```

### 16.3 调试引用与指针

常见错误：

- 空指针
- 悬空指针
- 对象生命周期结束后继续访问

重点看：

```gdb
p ptr
p *ptr
ptype ptr
```

### 16.4 虚函数和多态

如果你怀疑实际调用的对象类型和预期不一样，可以查看对象地址、类型信息、虚表附近内存。

初学阶段先掌握：

- 先确认指针是否为空
- 再确认实际对象是否还活着
- 再确认当前调用的是哪个实现

### 16.5 异常

有时程序不是段错误，而是异常终止。

可以先看调用栈，定位抛异常位置。

如果是 C++ 异常相关问题，GDB 也常配合日志一起排查。

---

## 17. 汇编视角和寄存器基础

大多数日常问题不需要你精通汇编，但看一点点会很有帮助。

### 17.1 查看当前函数反汇编

```gdb
disassemble
```

查看某个函数：

```gdb
disassemble main
```

### 17.2 查看当前 PC 附近指令

```gdb
x/10i $pc
```

### 17.3 查看寄存器

```gdb
info registers
```

### 17.4 什么时候需要汇编视角

- 栈损坏，源码信息不完整
- 想确认程序实际执行到哪条指令
- 分析优化编译后的行为
- 调试底层库、系统调用附近问题

对初学者来说，知道下面两条就够用：

- `$pc`：当前执行位置
- `x/10i $pc`：看当前附近指令

---

## 18. 共享库与源码不匹配的常见问题

很多人调试失败，不是不会命令，而是环境不对。

### 18.1 常见现象

- 断点打不上
- 行号对不上
- 变量看不全
- 栈信息奇怪

### 18.2 常见原因

#### 原因 1：没有调试符号

解决：重新用 `-g` 编译。

#### 原因 2：源码和运行二进制不是同一版

解决：确认你调试的是当前实际运行的那个二进制。

#### 原因 3：动态库版本不匹配

例如：

- 你编译时用的是一版 so
- 运行时实际加载的是另一版 so

#### 原因 4：优化过高

解决：开发调试阶段使用 `-O0`。

### 18.3 一个重要意识

**调试本质上是在分析“真实运行现场”，所以环境一致性非常重要。**

---

## 19. ROS 2 / Linux 开发中的常见用法

这一节更贴近你平时可能遇到的工程场景。

### 19.1 调试单个 ROS 2 节点

如果你有一个节点可执行文件，比如：

```bash
ros2 run my_pkg my_node
```

那你也可以直接：

```bash
gdb --args ros2 run my_pkg my_node
```

进入 GDB 后：

```gdb
r
```

### 19.2 调试 launch 启动的节点

更常见的办法是先单独启动出问题的节点，再用 GDB 跑它，避免 launch 里进程太多。

### 19.3 节点崩溃时的思路

如果节点直接退出：

1. 先确认是不是段错误
2. 用 GDB 直接跑节点
3. 崩溃后 `bt`
4. 看参数、对象生命周期、回调里访问了什么空指针

### 19.4 节点卡住时的思路

如果节点不退出但没响应：

1. 找 PID
2. `gdb -p PID`
3. `info threads`
4. `thread apply all bt`

### 19.5 机器人平台开发里常见的几类问题

#### 回调里空指针

比如：

- 订阅消息回调里访问了未初始化对象
- 定时器回调里用了已经析构的资源

#### 状态变量被改乱

适合：

```gdb
watch state
```

#### 多线程执行器相关问题

适合：

```gdb
info threads
thread apply all bt
```

#### 第三方库崩溃

关键看：

- 调用栈是谁先传了坏数据
- 崩点在库里，但问题是不是在你自己的输入

---

## 20. 推荐学习路线

不要一开始背所有命令。最好的方式是分阶段。

### 第一阶段：先能把程序停下来并看变量

目标：

- 会进 GDB
- 会打断点
- 会单步
- 会看变量

必学命令：

```gdb
b
r
n
s
c
p
l
```

### 第二阶段：能独立分析普通 bug

目标：

- 会看局部变量
- 会看参数
- 会看调用栈
- 会用条件断点

必学命令：

```gdb
info locals
info args
bt
frame
b ... if ...
```

### 第三阶段：能处理崩溃和卡住问题

目标：

- 会分析段错误
- 会看 core dump
- 会 attach 进程
- 会分析线程

必学命令：

```gdb
bt full
gdb ./app core
gdb -p PID
info threads
thread apply all bt
```

### 第四阶段：面向工程实战

目标：

- 调试大型 C++ 工程
- 调试 ROS 2 节点
- 调试库调用问题
- 结合日志和 GDB 一起排查

---

## 21. 练习题

下面这部分建议你真的自己敲一遍。

---

### 练习 1：最基本的断点和单步

代码：

```cpp
#include <iostream>

int add(int a, int b) {
    int c = a + b;
    return c;
}

int main() {
    int x = 1;
    int y = 2;
    int z = add(x, y);
    std::cout << z << std::endl;
    return 0;
}
```

要求：

1. 在 `main` 下断点
2. 运行程序
3. 单步走到 `add(x, y)` 这一行
4. 用 `step` 进入 `add`
5. 查看 `a`、`b`、`c` 的值
6. 用 `finish` 返回到 `main`

---

### 练习 2：条件断点

代码：

```cpp
#include <iostream>

int main() {
    int sum = 0;
    for (int i = 0; i < 10; ++i) {
        sum += i;
    }
    std::cout << sum << std::endl;
    return 0;
}
```

要求：

1. 只在 `i == 7` 时停下
2. 打印 `sum`
3. 继续运行到程序结束

---

### 练习 3：观察点

代码：

```cpp
#include <iostream>

int main() {
    int value = 10;
    value += 5;
    value *= 2;
    value -= 3;
    std::cout << value << std::endl;
    return 0;
}
```

要求：

1. 在 `main` 开头停下
2. 对 `value` 设置观察点
3. 每次 `value` 改变时查看新值

---

### 练习 4：调用栈

代码：

```cpp
#include <iostream>

int f3(int x) {
    return 10 / x;
}

int f2(int x) {
    return f3(x - 2);
}

int f1(int x) {
    return f2(x - 3);
}

int main() {
    int result = f1(5);
    std::cout << result << std::endl;
    return 0;
}
```

要求：

1. 运行程序直到崩溃
2. 查看调用栈
3. 找出哪一层导致除零
4. 分析参数是怎样一步步传坏的

---

### 练习 5：空指针崩溃

代码：

```cpp
#include <iostream>

struct Node {
    int value;
};

int main() {
    Node* p = nullptr;
    std::cout << p->value << std::endl;
    return 0;
}
```

要求：

1. 运行直到崩溃
2. 查看 `frame 0`
3. 查看 `p` 的值
4. 用一句话说明崩溃原因

---

### 练习 6：数组越界

代码：

```cpp
#include <iostream>

int main() {
    int arr[3] = {1, 2, 3};
    for (int i = 0; i <= 3; ++i) {
        std::cout << arr[i] << std::endl;
    }
    return 0;
}
```

要求：

1. 在循环里停下
2. 查看 `i`
3. 找出哪次访问越界
4. 尝试设置条件断点，只在越界那次停下

---

### 练习 7：修改变量验证假设

代码：

```cpp
#include <iostream>

int main() {
    int divisor = 0;
    int value = 100 / divisor;
    std::cout << value << std::endl;
    return 0;
}
```

要求：

1. 在除法前停住
2. 查看 `divisor`
3. 使用 `set var` 把它改成 `5`
4. 继续运行，观察程序是否正常

---

### 练习 8：core dump

要求：

1. 用你前面的崩溃程序之一生成 core
2. 用 `gdb ./app core` 打开
3. 用 `bt` 看调用栈
4. 和直接运行时的 GDB 调试结果对比

---

### 练习 9：attach 调试

要求：

1. 写一个无限循环程序，例如每秒打印一次数字
2. 在另一个终端里找到它的 PID
3. 用 `gdb -p PID` 附加
4. 查看当前调用栈
5. `detach` 后让程序继续跑

---

### 练习 10：多线程查看

代码示意（可自行补全）：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void worker1() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void worker2() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::thread t1(worker1);
    std::thread t2(worker2);
    t1.join();
    t2.join();
    return 0;
}
```

要求：

1. 运行后 attach 或直接在 GDB 中运行
2. 查看线程列表
3. 切换线程
4. 对所有线程执行 `bt`

---

### 练习 11：C++ 对象查看

代码：

```cpp
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;
};

int main() {
    Person p{"Alice", 20};
    std::cout << p.name << ", " << p.age << std::endl;
    return 0;
}
```

要求：

1. 在输出前停下
2. 打印整个对象 `p`
3. 分别打印 `p.name` 和 `p.age`

---

### 练习 12：综合题

代码：

```cpp
#include <iostream>
#include <vector>

int divide(int a, int b) {
    return a / b;
}

int main() {
    std::vector<int> nums = {10, 5, 0, 2};

    for (size_t i = 0; i < nums.size(); ++i) {
        int result = divide(100, nums[i]);
        std::cout << "result = " << result << std::endl;
    }

    return 0;
}
```

要求：

1. 在 `divide` 下断点
2. 每次进入 `divide` 查看 `a` 和 `b`
3. 找出程序在哪次循环出错
4. 改成条件断点，只在 `b == 0` 时停下
5. 用 `bt` 看调用路径
6. 用 `set var b = 1` 验证程序是否能继续跑过去

---

## 22. 练习题参考答案

这里只给方向，不建议一开始就直接看。

### 练习 1 参考

常用命令：

```gdb
b main
r
n
n
s
info args
info locals
finish
```

### 练习 2 参考

```gdb
b main.cpp:6 if i == 7
r
p sum
c
```

> 行号按你实际文件为准。

### 练习 3 参考

```gdb
b main
r
n
watch value
c
p value
c
```

### 练习 4 参考

关键命令：

```gdb
r
bt
frame 0
up
info args
```

分析思路：

- `main(5)` 调用 `f1(5)`
- `f1(5)` 调用 `f2(2)`
- `f2(2)` 调用 `f3(0)`
- 最后 `10 / 0` 崩溃

### 练习 5 参考

```gdb
r
frame 0
p p
```

结论：

- `p` 是空指针
- `p->value` 解引用空指针导致段错误

### 练习 6 参考

条件断点思路：

```gdb
b main.cpp:5 if i == 3
r
p i
p arr[i]
```

说明：

- `arr` 长度是 3
- 合法下标是 `0 1 2`
- `i == 3` 时已经越界

### 练习 7 参考

```gdb
b main.cpp:5
r
p divisor
set var divisor = 5
n
p value
c
```

### 练习 8 参考

```bash
ulimit -c unlimited
./app
gdb ./app core
```

进入后：

```gdb
bt
frame 0
info locals
```

### 练习 9 参考

```bash
gdb -p PID
```

进入后：

```gdb
bt
detach
q
```

### 练习 10 参考

```gdb
info threads
thread 2
bt
thread apply all bt
```

### 练习 11 参考

```gdb
b main
r
n
p p
p p.name
p p.age
```

### 练习 12 参考

```gdb
b divide
r
info args
c

# 改成条件断点
b divide if b == 0
r
bt
set var b = 1
c
```

---

## 23. 最终速查清单

把这一段记住，你就已经能开始实战了。

```gdb
b main              # 在 main 下断点
b 12                # 在第12行下断点
b file.cpp:20       # 在指定文件行下断点
b func if x == 0    # 条件断点
r                   # 运行
n                   # 单步，不进入函数
s                   # 单步，进入函数
finish              # 运行到当前函数返回
c                   # 继续执行
l                   # 查看源码
p var               # 打印变量
ptype var           # 查看类型
info locals         # 查看局部变量
info args           # 查看参数
bt                  # 查看调用栈
bt full             # 查看详细调用栈
frame 0             # 切到当前栈帧
up                  # 上一层栈帧
down                # 下一层栈帧
watch var           # 变量变化时停
x/10i $pc           # 查看当前指令
x/4xw &var          # 查看内存
set var x = 10      # 修改变量
info threads        # 查看线程
thread 2            # 切线程
thread apply all bt # 查看所有线程调用栈
detach              # 从进程脱离
q                   # 退出
```

---

## 最后建议

学 GDB 最容易踩的坑，不是命令不会，而是：

- 只背命令，不练场景
- 看到崩溃点就停，不继续向上追调用链
- 调试环境和运行环境不一致
- 二进制、源码、core 版本不匹配

真正正确的调试思路应该是：

**停住现场 → 看变量 → 看调用栈 → 找错误值来源 → 找最早异常点。**

你把这套流程练熟，GDB 就不再只是“会几个命令”，而会变成你真正的排错工具。

