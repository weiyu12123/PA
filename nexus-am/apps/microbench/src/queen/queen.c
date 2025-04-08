#include <benchmark.h>

static unsigned int FULL;

static unsigned int dfs(unsigned int row, unsigned int ld, unsigned int rd) {
  if (row == FULL) {
    return 1;
  } else {
    unsigned int pos = FULL & (~(row | ld | rd)), ans = 0;
    
    // 打印当前状态
    printf("dfs(row=0x%x, ld=0x%x, rd=0x%x) -> pos=0x%x\n", row, ld, rd, pos);
    
    while (pos) {
      unsigned int p = (pos & (~pos + 1));
      pos -= p;
      
      // 打印每次选中的位置 p
      printf("  Trying position p=0x%x (row=0x%x, ld=0x%x, rd=0x%x)\n", p, row, ld, rd);
      
      ans += dfs(row | p, (ld | p) << 1, (rd | p) >> 1);
    }

    // 打印返回的答案
    printf("Returning from dfs(row=0x%x, ld=0x%x, rd=0x%x): ans=%u\n", row, ld, rd, ans);
    
    return ans;
  }
}

static unsigned int ans;

void bench_queen_prepare() {
  ans = 0;
  FULL = (1 << setting->size) - 1;
  printf("bench_queen_prepare: FULL=0x%x\n", FULL);  // 打印FULL的值
}

void bench_queen_run() {
  printf("Running dfs...\n");
  ans = dfs(0, 0, 0);
  printf("Completed dfs. Result: %u\n", ans);
}

int bench_queen_validate() {
  printf("Validating result: expected checksum=0x%x, computed ans=%u\n", setting->checksum, ans);
  return ans == setting->checksum;
}
