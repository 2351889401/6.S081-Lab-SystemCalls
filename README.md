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
![](https://github.com/2351889401/6.S081-Lab-SystemCalls/blob/main/images/systrace.png)  
  
主要想解释下图中红色方框中使用的命令行参数，为什么会是上面的运行结果。程序的执行流程如下：  
（1）命令行程序（“**shell程序**”，代码在“**user/sh.c**”）执行的死循环为：
```
// Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(2, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(0);
  }
```
主要是先“**fork1()**”子进程，然后“**runcmd()**”运行命令行指示的程序。显然输入命令行参数后，这里应当运行用户程序“**user/trace.c**”。  
   
（2）“**user/trace.c**”中的主函数如下
```
int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
```
可以看到，先执行了系统调用“**trace()**”，然后去执行“**exec()**”修改当前进程的内容，接着去执行后面的命令行参数指向的用户程序。  

我们的“**sys_trace()**”主要的任务是为我们定义的“**trace_num**”赋值。  
现在“**trace_num**”值为“**2147483647**”，是“**(1<<31)-1**”，也就是会追踪这之后的所有的系统调用（系统调用的最大编号20多）。  
在“**sys_trace()**”执行完毕后，回到了“**kernel/syscall.c/syscall()**”函数里：
```
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  // printf("num = %d\n", num);
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num](); //系统调用在这里结束
    if(p->trace_num & (1<<num)) printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```
此时的num给的是“**sys_trace()**”的系统调用号，所以会执行下面的输出语句。  
  
当“**user/trace.c**”中的主函数中“**trace()**”的部分执行完毕后，执行下面的语句：
```
exec(nargv[0], nargv);
```
所以上面图中会有“**syscall exec**”的输出。  

“**exec()**”会将进程的原始内存、页表释放，建立新进程的内存、页表等，然后去执行新进程。很显然这里的新进程是“**grep hello README**”，大意就是“去**README**文件中过滤出**hello**的字符串”。所以我们现在的执行流程到了“**user/grep.c**”。  
```
void
grep(char *pattern, int fd)
{
  int n, m;
  char *p, *q;

  m = 0;
  while((n = read(fd, buf+m, sizeof(buf)-m-1)) > 0){
    m += n;
    buf[m] = '\0';
    p = buf;
    while((q = strchr(p, '\n')) != 0){
      *q = 0;
      if(match(pattern, p)){
        *q = '\n';
        write(1, p, q+1 - p);
      }
      p = q+1;
    }
    if(m > 0){
      m -= p - buf;
      memmove(buf, p, m);
    }
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;
  char *pattern;

  if(argc <= 1){
    fprintf(2, "usage: grep pattern [file ...]\n");
    exit(1);
  }
  pattern = argv[1];

  if(argc <= 2){
    grep(pattern, 0);
    exit(0);
  }

  for(i = 2; i < argc; i++){ //这里argc是3 所以会执行到这里
    if((fd = open(argv[i], 0)) < 0){
      printf("grep: cannot open %s\n", argv[i]);
      exit(1);
    }
    grep(pattern, fd);
    close(fd);
  }
  exit(0);
}
```
所以会依次执行“**open**”、“**read**”、“**close**”系统调用。（没有执行“**grep**”函数中的“**write**”系统调用是因为**README**文件里面没有“**hello**”）。  
至此，分析完毕。  
  
**2.** 添加系统调用“**sysinfo()**”，调用该系统调用时，会将系统的即时信息返回给用户空间的地址（通过“**kernel/vm.c/copyout()**”将内核中的数据拷贝给用户空间的地址，这里的用意大概是为了下一节“**页表**”的内容做准备）  
  
（这里同样略去细节）主要给出修改的代码：  
**kernel/proc.c**中增加函数“**get_unused_num()**”获取当前没有处于**UNUSED**状态的进程数量  
```
//收集有多少进程不处于UNUSED状态
uint64 get_unused_num(void)
{
  // char *sstates[] = {
  // [UNUSED]    "unused",
  // [SLEEPING]  "sleep ",
  // [RUNNABLE]  "runble",
  // [RUNNING]   "run   ",
  // [ZOMBIE]    "zombie"
  // };
  struct proc *p;
  uint64 num = 0;
  for(p=proc; p < &proc[NPROC]; p++) {
    // printf("pid:%d state:%s\n", p->pid, sstates[p->state]);
    acquire(&p->lock);
    if(p->state == UNUSED) {
      release(&p->lock);
      continue;
    }
    else num++;
    release(&p->lock);
  }
  // printf("totalnum is %d\n", num);
  return num;
}
```

**kernel/kalloc.c**中增加函数“**get_freemem()**”计算当前的空闲内存  
这里有2种实现：第1种是最早实现的方式，当时不了解内存的信息，为了做出来，想了一些偏招（哈哈哈）；第2种是写这篇文章的时候写的，这时候实际上已经完成了5个实验了，对内存这块了解的更深刻了。  

第1种的想法和下面注释写的一样：不知道怎么处理的情况下，就逐渐增加内存，直到不能再增加；结束后别忘了释放内存（因为当时大概看过“**growproc**”函数似乎和内存大小有关）
```
uint64 get_freemem(void)
{
  // printf("hello this is get freemem\n");
  uint n = 0;

  //怎么样去统计free memory? 模仿用户程序 逐渐增加当前进程的memory 直到不能再增加为止 之后不要忘了回退之前的内存增加的操作
  while(growproc(PGSIZE) != -1) {
    n += PGSIZE;
  }

  growproc(-n);

  return n;
}
```
第2种就很简单啦：直接链表遍历
```
uint64 get_freemem(void) {
  uint n = 0;
  struct run *r = kmem.freelist;

  while(r) {
    n += PGSIZE;
    r = r->next;
  }

  return n;
}
```

**kernel/sysproc.c**中增加系统调用函数“**sys_sysinfo()**”将内核获取到的当前系统信息传递给用户空间
```
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

  //首先需要获取用户程序提供的地址 进入内核前存储在a0(寄存器)里面
  uint64 user_addr;
  if(argaddr(0, &user_addr) < 0) return -1;

  // printf("useraddr: %d\n", user_addr);

  p = myproc();
  if(copyout(p->pagetable, user_addr, (char *)&info, sizeof(struct sysinfo)) < 0) return -1;

  return 0;
}
```
至此，完成实验，结果如下：  
这里具体的一些细节可以参考源码，难度不大，直接给出测试结果：  
![](https://github.com/2351889401/6.S081-Lab-SystemCalls/blob/main/images/sysinfo.png)  
