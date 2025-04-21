#include "common.h"
#include "syscall.h"

int sys_none() {
  return 1;
}

void sys_exit(int a){
  _halt(a);
}

uintptr_t sys_write(int fd, const void *buf, size_t count){
  uintptr_t i = 0;

  if (fd == 1 || fd == 2) { // stdout or stderr
    for (; i < count; i++) {
      _putc(((char *)buf)[i]); // _putc 是 AM 提供的字符输出函数
    }
    return count; // 成功写入的字节数
  }

  return -1; // 对于不支持的 fd，返回错误
}


_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none: 
      SYSCALL_ARG1(r) = sys_none();
      break;
    case SYS_exit: 
      sys_exit(a[1]);
      break;
    case SYS_write:
      SYSCALL_ARG1(r) = sys_write(a[1], (void*)a[2], a[3]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
