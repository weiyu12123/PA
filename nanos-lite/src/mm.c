#include "proc.h"
#include "memory.h"

static void *pf = NULL;

void* new_page(void) {
  assert(pf < (void *)_heap.end);
  void *p = pf;
  pf += PGSIZE;
  return p;
}

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler */
int mm_brk(uint32_t new_brk) {
    if (current->cur_brk == 0) {
        // 初始化时设置当前和最大brk为new_brk
        current->cur_brk = current->max_brk = new_brk;
    } else {
        if (new_brk > current->max_brk) {
            // 需要分配新的页
            uint32_t first = PGROUNDUP(current->max_brk);
            uint32_t end = PGROUNDDOWN(new_brk);

            // 如果new_brk正好对齐，那么end少一页（因为PGROUNDDOWN不会覆盖new_brk本身）
            if ((new_brk & 0xfff) == 0) {
                end -= PGSIZE;
            }

            for (uint32_t va = first; va <= end; va += PGSIZE) {
                void *pa = new_page();  // 分配一页物理内存
                _map(&(current->as), (void *)va, pa);  // 映射到虚拟地址空间
            }

            current->max_brk = new_brk;
        }

        // 无论是扩展还是收缩，都更新cur_brk
        current->cur_brk = new_brk;
    }
    return 0;
}


void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start);
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page);
}
