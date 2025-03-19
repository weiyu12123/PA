#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

void init_regex();
/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *);    
static int cmd_info(char *);  
static int cmd_p(char *);     
static int cmd_x(char *);     
static int cmd_w(char *);     
static int cmd_d(char *);     

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* 单步执行 */
  { "si",   "Single-step execution [N instructions] (default N=1)", cmd_si },
  
  /* 打印程序状态 */
  { "info", "Print program state", cmd_info },
  
  /* 表达式求值 */
  { "p",    "Evaluate expression", cmd_p },
  
  /* 扫描内存 */
  { "x",    "Examine memory", cmd_x },
  
  /* 监视点操作 */
  { "w",    "Set watchpoint", cmd_w },
  { "d",    "Delete watchpoint", cmd_d },
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  int n = 1;  // Default: execute 1 instruction
  if (args != NULL) {
    n = atoi(args);  // Convert argument to integer
    if (n <= 0) {
      printf("Error: the number of steps must be greater than 0.\n");
      return 0;
    }
  }
  cpu_exec(n);  // Call the CPU execution function with n steps
  return 0;
}

static int cmd_info(char *args) {
  char c;
  if (args == NULL) {
    printf("Invalid arguement.\n");
    return 0;
  }
  if (sscanf(args, "%c", &c) <= 0) {
    printf("Invalid arguement.\n");
    return 0;
  }

  if (c == 'r') {
    // DWORD
    for (int i=0; i<8; i++)
      printf("%s   0x%x\n", regsl[i], reg_l(i));
    printf("eip   0x%x\n", cpu.eip);
    // WORD
    for (int i=0; i<8; i++)
      printf("%s    0x%x\n", regsw[i], reg_w(i));
    // BYTE
    for (int i=0; i<8; i++)
      printf("%s    0x%x\n", regsb[i], reg_b(i));
  } else if (c == 'w') {
    print_wp();
  } else {
    printf("Invalid arguement.\n");
  }
  return 0;
}

static int cmd_x(char *args) {
    if (!args) {
        printf("args error in cmd_%s\n", "x");
        return 0;
    }
    
    char *args_end = args + strlen(args);
    char *first_args = strtok(NULL, " ");
    
    if (!first_args) {
        printf("args error in cmd_%s\n", "x");
        return 0;
    }
    
    char *exprs = first_args + strlen(first_args) + 1;
    if (exprs >= args_end) {
        printf("args error in cmd_%s\n", "x");
        return 0;
    }
    
    int n = atoi(first_args);
    bool success;
    vaddr_t addr = expr(exprs, &success);
    
    if (success == false) {
        printf("error in expr()\n");
        printf("Memory:\n");
        return 0;
    }
    
    for (int i = 0; i < n; i++) {
        printf("0x%x:", addr);
        uint32_t val = vaddr_read(addr, 4);
        uint8_t *by = (uint8_t *)&val;
        
        printf("0x");
        for (int j = 3; j >= 0; j--) {
            printf("%02x", by[j]);
        }
        printf("\n");
        
        addr += 4;
    }
    
    return 0;
}


static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Input invalid command! Please input the expression.\n");
  }
  else {
    init_regex();

    bool success = true;
    //printf("args = %s\n", args);
    int result = expr(args, &success);

    if (success) {
      printf("result = %d\n", result);
    }
    else {
      printf("Invalid expression!\n");
    }
  }
  return 0;
}

static int cmd_w(char *args) {
  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  int num = 0;
  if (sscanf(args, "%d", &num) <= 0) {
    printf("Invalid argument.\n");
    return 0;
  }

  if (free_wp(num))
    printf("Successfully deleted watchpoint %d\n", num);
  else
    printf("Error: no watchpoint %d\n", num);
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
