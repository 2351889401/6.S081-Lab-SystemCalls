### 系统调用实验应该是前期非常基础的内容了，这里也只是简单介绍下实验内容。

**1.** 增加系统调用“**trace()**”。如果一个进程调用了它，那么该进程之后调用的系统调用（如果在“**trace()**”中设置了被追踪）会被追踪。  

（这里太细节的内容就略过了，可以参考源码）  
**kernel/proc.h**中添加：
```
int trace_num; //保存trace的系统调用的bit
```
**kernel/sysproc.c**中添加函数“**sys_trace()**”：
```
uint64
sys_trace(void)
{
  int n;
  struct proc* p = myproc();
  if(argint(0, &n) < 0) return -1; //这里还可以通过 p->trapframe->a0 获取
  p->trace_num = n;
  return 0;
}
```
**kernel/proc.c**中的“**fork()**”中添加：
```
np->trace_num = p->trace_num; // copy trace mask
```
**kernel/syscall.c**中的“**syscall()**”中添加：
```
if(p->trace_num & (1<<num)) printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);
```
  
这里具体的一些细节可以参考源码，难度不大，直接给出测试结果：  
![]()
