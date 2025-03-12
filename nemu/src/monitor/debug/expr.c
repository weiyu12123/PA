#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV,
  TK_LPAREN, TK_RPAREN,TK_NEG

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
  {"[0-9]+", TK_NUM},       // 数字
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
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //    i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        if (rules[i].token_type == TK_NOTYPE) break;  // 忽略空格
        assert(nr_token < 32);

        tokens[nr_token].type = rules[i].token_type;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NUM:
            if (substr_len < 32) {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
            } else {
              printf("Error: Number token too long\n");
              return false;
            }
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
        case TK_NEG: return 2; // 一元负号优先级最高
        case TK_MUL:
        case TK_DIV: return 3;
        case TK_PLUS:
        case TK_MINUS: return 4;
        case TK_EQ: return 7;
        default: return 100;
    }
}

/* 在 tokens[p...q] 中找到主运算符的位置.
 * 主运算符是优先级最低的运算符（不在括号内部的），如果存在多个，则选择最右边的一个。
 */
// 查找主运算符
int find_main_operator(int p, int q) {
    int op = -1;
    int min_priority = 100;
    int balance = 0;

    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) {
            balance++;
            continue;
        }
        if (tokens[i].type == TK_RPAREN) {
            balance--;
            continue;
        }
        if (balance > 0) continue;

        int pri = priority(tokens[i].type);
        if (pri <= min_priority) {
            min_priority = pri;
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
        if (tokens[p].type == TK_NUM)
            return atoi(tokens[p].str);
    }

    if (tokens[p].type == TK_NEG) {
        return -eval(p + 1, q);
    } else if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1);
    } else {
        int op = find_main_operator(p, q);
        int val1 = eval(p, op - 1);
        int val2 = eval(op + 1, q);
        switch (tokens[op].type) {
            case TK_PLUS:  return val1 + val2;
            case TK_MINUS: return val1 - val2;
            case TK_MUL:   return val1 * val2;
            case TK_DIV: 
                assert(val2 != 0);
                return val1 / val2;
            default: assert(0);
        }
    }
    return 0;
}


void convert_minus_to_neg() {
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == TK_MINUS) {
            // 如果 '-' 在表达式开始，或者前一个 token 不是数字和右括号，则为一元负号
            if (i == 0 ||
                (tokens[i - 1].type != TK_NUM &&
                 tokens[i - 1].type != TK_RPAREN)) {
                tokens[i].type = TK_NEG;
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
    convert_minus_to_neg();
    *success = true;
    return eval(0, nr_token - 1);
}
