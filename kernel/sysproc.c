#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#include "sysinfo.h"
extern uint64 get_unused_num(void);
extern uint64 get_freemem(void);

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64
sys_trace(void)
{
  int n;
  struct proc* p = myproc();
  // argint(0, &n);
  // printf("in sys_trace get n = %d\n", n);
  if(argint(0, &n) < 0) return -1;
  p->trace_num = n;
  // printf("ok you get it! %d\n", p->trace_num);
  return 0;
}

uint64
sys_sysinfo(void)
{
  struct proc *p;
  struct sysinfo info;
  uint64 num_process;
  uint64 num_freemem;

  // printf("hello this is sysinfo!\n");
  num_process = get_unused_num();
  num_freemem = get_freemem();

  info.freemem = num_freemem;
  info.nproc = num_process;
  //将内核数据拷贝到用户程序提供的地址

  //首先需要获取用户程序提供的地址 进入内核前存储在a0(寄存器)里面
  uint64 user_addr;
  if(argaddr(0, &user_addr) < 0) return -1;

  // printf("useraddr: %d\n", user_addr);

  p = myproc();
  if(copyout(p->pagetable, user_addr, (char *)&info, sizeof(struct sysinfo)) < 0) return -1;

  return 0;
}