#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[256];              // 被监视的表达式
  int old_value;          // 之前计算的值
  int hit_num;
  /* TODO: Add more members if necessary */
  
} WP;

bool new_wp(char *args);
bool free_wp(int num);
void print_wp();
bool watch_wp();

#endif
