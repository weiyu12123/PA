#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *watchpoint_list, *free_list;
static int next_wp_no;
static WP* current_wp;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].old_value = 0;
    wp_pool[i].hit_num = 0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  watchpoint_list = NULL;
  free_list = wp_pool;
  next_wp_no = 0;
}

bool new_wp(char *args) {
  if (free_list == NULL)
    assert(0);
  
  WP* wp = free_list;
  free_list = free_list->next;

  wp->NO = next_wp_no++;
  wp->next = NULL;
  strcpy(wp->expr, args);  
  wp->hit_num = 0;

  bool valid_expr;
  wp->old_value = expr(wp->expr, &valid_expr);
  if (!valid_expr) {
    printf("Syntax error.\n");
    return false;
  }

  current_wp = watchpoint_list;
  if (current_wp == NULL) {
    watchpoint_list = wp;
  } else {
    while (current_wp->next != NULL)
      current_wp = current_wp->next;
    current_wp->next = wp;
  }

  printf("Success: set watchpoint %d, old_value=%d\n", wp->NO, wp->old_value);
  return true;
}

bool free_wp(int num) {
  WP* the_wp = NULL;
  if (watchpoint_list == NULL) {
    printf("No watchpoint now\n");
    return false;
  }

  if (watchpoint_list->NO == num) {
    the_wp = watchpoint_list;
    watchpoint_list = watchpoint_list->next;
  } else {
    current_wp = watchpoint_list;
    while (current_wp != NULL && current_wp->next != NULL) {
      if (current_wp->next->NO == num) {
        the_wp = current_wp->next;
        current_wp->next = current_wp->next->next;
        break;
      }
      current_wp = current_wp->next;
    }
  }

  if (the_wp != NULL) {
    the_wp->next = free_list;
    free_list = the_wp;
    return true;
  }
  return false;
}

void print_wp() {
  if (watchpoint_list == NULL) {
    printf("No watchpoint now\n");
    return;
  }
  printf("Watchpoints:\n");
  printf("NO.   expr            hit_num\n");
  current_wp = watchpoint_list;
  while (current_wp) {
    printf("%d     %s          %d\n", current_wp->NO, current_wp->expr, current_wp->hit_num);
    current_wp = current_wp->next;
  }
}

bool watch_wp() {
  bool success;
  int current_value;
  if (watchpoint_list == NULL)
    return true;
  
  current_wp = watchpoint_list;
  while (current_wp) {
    current_value = expr(current_wp->expr, &success);
    if (current_value != current_wp->old_value) {
      current_wp->hit_num += 1;
      printf("Hardware watchpoint %d:%s\n", current_wp->NO, current_wp->expr);
      printf("Old_value:%d\nNew_value:%d\n\n", current_wp->old_value, current_value);
      current_wp->old_value = current_value;
      return false;
    }
    current_wp = current_wp->next;
  }
  return true;
}
