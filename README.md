## 1. System call tracing 中等

https://github.com/xinwuyun/MIT-6.s081-lab/commit/521fb3f8a13c6c6a49a3197a12937ea7763e34c0

在本作业中，您将添加一个系统调用跟踪特性，这在调试以后的实验时可能会对您有所帮助。您将创建一个新的跟踪系统调用来控制跟踪。它应该有一个参数，一个整数“掩码”，它的位指定要跟踪哪个系统调用。

例如，要跟踪fork系统调用，程序调用trace(1 << SYS fork)，其中SYS fork是一个系统调用号，在`kernel/syscall.h`中

如果掩码中设置了系统调用的编号，则必须修改xv6内核，以便在每个系统调用即将返回时打印一行。该行应该包含**进程id、系统调用的名称和返回值**;您不需要打印系统调用参数。跟踪系统调用应该支持对调用它的进程和它随后派生的任何子进程的跟踪，但不应该影响其他进程。

```shell
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0
$
$ trace 2147483647 grep hello README
4: syscall trace -> 0
4: syscall exec -> 3
4: syscall open -> 3
4: syscall read -> 1023
4: syscall read -> 966
4: syscall read -> 70
4: syscall read -> 0
4: syscall close -> 0
$
$ grep hello README
$
$ trace 2 usertests forkforkfork
usertests starting
test forkforkfork: 407: syscall fork -> 408
408: syscall fork -> 409
409: syscall fork -> 410
410: syscall fork -> 411
409: syscall fork -> 412
410: syscall fork -> 413
409: syscall fork -> 414
411: syscall fork -> 415
...
$   
```

提示

1. 编写一个 `user/trace.c`，当然也要修改 `Makefile`;
2. 修改 `kernel/syscall.h`，`user/user.h` 和 `user/usys.pl`
3. 在`kernel/sysproc.c`中添加 `sys_trace()`，可以在`kernel/sysproc.c`看到类似用法。实现记住`mask`，放到`proc`结构体里
4. 从用户空间检索系统调用参数的函数在`syscall`里
5. 修改`fork()`copy the trace mask from the parent to the child process.
6. 修改 `syscall`，打印 trace 的输出

### 结果

![image-20220707002511521](https://cdn.jsdelivr.net/gh/xinwuyun/pictures@main/2022/07/07/1b604b85080f80f85a0322ad3d47a5a5-image-20220707002511521-09673a.png)

`fork`中复制mask

```c
// proc.c
int
fork(void)
{
  ...
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // 加了这句
  // Copy mask from parent to child
  np->mask = p->mask;

  ...
  return pid;
}
```



添加`uint32 mask`

```c
// proc.h
// Per-process state
struct proc {
  struct spinlock lock;

  ...
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  uint32 mask;                 // Process trace mask
};
```

`syscall.c`添加

```c
// add this
extern uint64 sys_trace(void);
```

`sysproc.c`添加

```c
uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0)
    return -1;
  myproc()->mask = mask;
  return 0;
}
```

编写`user/trace.c`

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2){
    printf("Too few arguments.\n");
    exit(1);
  }
  int mask = atoi(argv[1]);
  if(trace(mask) !=0 ){
    printf("trace failed...\n");
    exit(1);
  }else{
    exec(argv[2], argv+2);
    printf("exec failed...\n");
    exit(1);
  }
  exit(0);
}
```

`user.h`中添加

```c
int trace(int);
```

`usys.pl`中添加

```c
entry("trace");
```

### 运行结果

![image-20220707002427464](https://cdn.jsdelivr.net/gh/xinwuyun/pictures@main/2022/07/07/5c0dc4f49e0c91a5647af6a6b8c405be-image-20220707002427464-9f1d2e.png)

### system call流程

> 引用https://blog.miigon.net/posts/s081-lab2-system-calls/

#### 如何创建新系统调用

1. 首先在内核中合适的位置（取决于要实现的功能属于什么模块，理论上随便放都可以，只是主要起归类作用），实现我们的内核调用（在这里是 trace 调用）：

   ```c
   // kernel/sysproc.c
    // 这里着重理解如何添加系统调用，对于这个调用的具体代码细节在后面的部分分析
    uint64
    sys_trace(void)
    {
    int mask;
   
    if(argint(0, &mask) < 0)
        return -1;
   	
    myproc()->syscall_trace = mask;
    return 0;
    }
   ```

   这里因为我们的系统调用会对进程进行操作，所以放在 sysproc.c 较为合适。

2. 在 syscall.h 中加入新 system call 的序号：

3. 用 extern 全局声明新的内核调用函数，并且在 syscalls 映射表中，加入从前面定义的编号到系统调用函数指针的映射

   这里 `[SYS_trace] sys_trace` 是 C 语言数组的一个语法，**表示以方括号内的值作为元素下标**。比如 `int arr[] = {[3] 2333, [6] 6666}` 代表 arr 的下标 3 的元素为 2333，下标 6 的元素为 6666，其他元素填充 0 的数组。（该语法在 C++ 中已不可用）

4. 在 usys.pl 中，加入用户态到内核态的跳板函数。

   `entry("trace"); `

   <u>这个脚本在运行后会生成 usys.S 汇编文件，里面定义了每个 system call 的用户态跳板函数：</u>

   ```asm
   trace:		# 定义用户态跳板函数
    li a7, SYS_trace	# 将系统调用 id 存入 a7 寄存器
    ecall				# ecall，调用 system call ，跳到内核态的统一系统调用处理函数 syscall()  (syscall.c)
    ret
   ```

5. 在用户态的头文件加入定义，使得用户态程序可以找到这个跳板入口函数。

   ```c
   int trace(void);
   ```

#### 系统调用全流程

```
user/user.h:		用户态程序调用跳板函数 trace()
user/usys.S:		跳板函数 trace() 使用 CPU 提供的 ecall 指令，调用到内核态
kernel/syscall.c	到达内核态统一系统调用处理函数 syscall()，所有系统调用都会跳到这里来处理。
kernel/syscall.c	syscall() 根据跳板传进来的系统调用编号，查询 syscalls[] 表，找到对应的内核函数并调用。
kernel/sysproc.c	到达 sys_trace() 函数，执行具体内核操作
```

这么繁琐的调用流程的主要目的是实现用户态和内核态的良好隔离。

并且由于内核与用户进程的页表不同，寄存器也不互通，所以参数无法直接通过 C 语言参数的形式传过来，而是需要使用 argaddr、argint、argstr 等系列函数，从进程的 trapframe 中读取用户进程寄存器中的参数。

同时由于页表不同，指针也不能直接互通访问（也就是内核不能直接对用户态传进来的指针进行解引用），而是需要使用 copyin、copyout 方法结合进程的页表，才能顺利找到用户态指针（逻辑地址）对应的物理内存地址。（在本 lab 第二个实验会用到）

```
struct proc *p = myproc(); // 获取调用该 system call 的进程的 proc 结构
copyout(p->pagetable, addr, (char *)&data, sizeof(data)); // 将内核态的 data 变量（常为struct），结合进程的页表，写到进程内存空间内的 addr 地址处。
```

## 2. Sysinfo

In this assignment you will add a system call, `sysinfo`, that collects information about the running system. The system call takes one argument: a pointer to a `struct sysinfo` (see `kernel/sysinfo.h`). The kernel should fill out the fields of this struct: the `freemem` field should be set to the number of bytes of free memory, and the `nproc` field should be set to the number of processes whose `state` is not `UNUSED`. We provide a test program `sysinfotest`; you pass this assignment if it prints "sysinfotest: OK".

https://github.com/xinwuyun/MIT-6.s081-lab/commit/ce1e740f5f5ae6d60744973147781df5188c350e