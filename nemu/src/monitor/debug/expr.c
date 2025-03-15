#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV,
  TK_LPAREN, TK_RPAREN,TK_NEG,TK_HEX,TK_REG,TK_NEQ,TK_AND,TK_OR,TK_NOT,TK_DEREF

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},         // equal
  {"\\-", TK_MINUS},         // 减号
  {"\\*", TK_MUL},         // 乘号
  {"\\/", TK_DIV},           // 除号
  {"\\(", TK_LPAREN},      // 左括号
  {"\\)", TK_RPAREN},      // 右括号
  {"0x[1-9A-Fa-f][0-9A-Fa-f]*",TK_HEX}, // 十六进制数
  {"[0-9]+", TK_NUM},       // 数字
  {"!=", TK_NEQ},           // 不等
  {"&&", TK_AND},           // 逻辑与
  {"\\|\\|", TK_OR},        // 逻辑或
  {"!", TK_NOT},            // 逻辑非
  {"\\*", TK_DEREF},        // 解引用操作符
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh|)",TK_REG}, //寄存器
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;
                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;

                if (rules[i].token_type == TK_NOTYPE) break;  // 忽略空格
                assert(nr_token < 32);

                tokens[nr_token].type = rules[i].token_type;

                switch (rules[i].token_type) {
                    case TK_NUM:
                        strncpy(tokens[nr_token].str,substr_start,substr_len);
                        *(tokens[nr_token].str+substr_len)='\0';
                        break;
                    case TK_HEX:
                        strncpy(tokens[nr_token].str,substr_start+2,substr_len-2);
                        *(tokens[nr_token].str+substr_len-2)='\0';
                        break;
                    case TK_REG:
                        strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
                        *(tokens[nr_token].str+substr_len-1)='\0';
                        break;
                    default:
                        tokens[nr_token].str[0] = substr_start[0];
                        tokens[nr_token].str[1] = '\0';
                        break;
                }
                nr_token++;
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}


/* 检查 tokens[p...q] 是否被一对匹配的括号包围.
 * 如果整个子表达式以一个左括号开始、以一个右括号结束，并且这对括号确实匹配，则返回 true.
 * 否则返回 false.
 */
bool check_parentheses(int p, int q) {
    if (tokens[p].type != TK_LPAREN || tokens[q].type != TK_RPAREN)
        return false;

    int balance = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) balance++;
        else if (tokens[i].type == TK_RPAREN) balance--;
        if (balance == 0 && i < q) return false;
    }
    return (balance == 0);
}

int priority(int type) {
    switch (type) {
        case TK_NEG:  // 一元负号
        case TK_NOT:  // 逻辑非
            return 0;  // 最高优先级
        case TK_MUL:
        case TK_DIV:  // 乘法和除法
            return 2;  
        case TK_PLUS:
        case TK_MINUS:  // 加法和减法
            return 3;
        case TK_EQ:
        case TK_NEQ:  // 相等和不等
            return 6;
        case TK_AND:  // 逻辑与
            return 7;
        case TK_OR:  // 逻辑或
            return 8;
        case TK_DEREF:  // 解引用
            return 1;  // 次高优先级，通常解引用优先于加减乘除
        default:
            return 100;  // 默认优先级，表示没有有效运算符
    }
}



/* 在 tokens[p...q] 中找到主运算符的位置.
 * 主运算符是优先级最低的运算符（不在括号内部的），如果存在多个，则选择最右边的一个。
 */
// 查找主运算符
int find_main_operator(int p, int q) {
    int op = -1;
    int max_priority = -1;
    int balance = 0;

    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) balance++;
        else if (tokens[i].type == TK_RPAREN) balance--;
        if (balance != 0) continue;

        // 确保只选择一元操作符（如负号、逻辑非）和二元操作符
        switch (tokens[i].type) {
            case TK_NEG:  // 一元负号
            case TK_NOT:  // 逻辑非
            case TK_PLUS:
            case TK_MINUS:
            case TK_MUL:
            case TK_DIV:
            case TK_EQ:
            case TK_NEQ:
            case TK_AND:
            case TK_OR:
            case TK_DEREF:  // 解引用
                break;
            default:
                continue;
        }

        int cur_priority = priority(tokens[i].type);
        // 如果当前运算符优先级更低，或优先级相同但位置更右，则更新主运算符
        if (cur_priority > max_priority || (cur_priority == max_priority && i > op)) {
            max_priority = cur_priority;
            op = i;
        }
    }

    return op;
}



/* 递归求值 tokens[p...q] 所表示的表达式.
 * 在这里，我们采用递归下降的方式，对表达式进行求值.
 */
int eval(int p, int q) {
    if (p > q) {
        assert(0);
    } else if (p == q) {
        int num;
        switch (tokens[p].type) {
            case TK_NUM:
                sscanf(tokens[p].str, "%d", &num);
                return num;
            case TK_HEX:
                sscanf(tokens[p].str, "%x", &num);
                return num;
            case TK_REG:
                for (int i = 0; i < 8; i++) {
                    if (strcmp(tokens[p].str, regsl[i]) == 0)
                        return reg_l(i);
                    if (strcmp(tokens[p].str, regsw[i]) == 0)
                        return reg_w(i);
                    if (strcmp(tokens[p].str, regsb[i]) == 0)
                        return reg_b(i);
                }
                if (strcmp(tokens[p].str, "eip") == 0)
                    return cpu.eip;
                else {
                    printf("error in TK_REG in eval()\n");
                    assert(0);
                }
        }
    }
    if (p < q) {
      if (check_parentheses(p, q)) {
          return eval(p + 1, q - 1);
      }else{
          int op = find_main_operator(p, q);
          vaddr_t addr;
          int result;

          switch (tokens[op].type) {
              case TK_NEG:
                  return -eval(p + 1, q);
              case TK_DEREF:
                  addr = eval(p + 1, q);
                  result = vaddr_read(addr, 4);
                  printf("addr=%u(0x%x) value=%d(0x%08x)\n", addr, addr, result, result);
                  return result;
              case '!':
                  result = eval(p + 1, q);
                  return result != 0 ? 0 : 1;
          }

          int val1 = eval(p, op - 1);
          int val2 = eval(op + 1, q);

          switch (tokens[op].type) {
              case TK_PLUS:   return val1 + val2;
              case TK_MINUS:  return val1 - val2;
              case TK_MUL:    return val1 * val2;
              case TK_DIV:    assert(val2 != 0); return val1 / val2;
              case TK_EQ:     return val1 == val2;
              case TK_NEQ:    return val1 != val2;
              case TK_AND:    return val1 && val2;
              case TK_OR:     return val1 || val2;
              default:        assert(0);  // Shouldn't reach here
          }
      }
    }
    return 0;
}

void convert_minus_and_deref() {
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == TK_MINUS) {
            if (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_RPAREN)) {
                tokens[i].type = TK_NEG;
            }
        }
        if (tokens[i].type == TK_MUL) {
            if (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_RPAREN)) {
                tokens[i].type = TK_DEREF;
            }
        }
    }
}



/* 对输入的表达式字符串 e 进行求值.
 * 如果表达式合法，则返回计算结果，同时通过 success 返回 true;
 * 否则返回 0，并将 success 设置为 false.
 *
 * 为了支持一元负号，在 make_token() 之后，还需要遍历 token 数组，
 * 将那些在表达式开头或者紧跟运算符或左括号的 '-' 转换为 TK_NEG.
 */
uint32_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    // 处理 `-` 和 `*`
    if (tokens[0].type == TK_MINUS) tokens[0].type = TK_NEG;
    if (tokens[0].type == TK_MUL) tokens[0].type = TK_DEREF;

    convert_minus_and_deref();

    *success = true;
    return eval(0, nr_token - 1);
}
