#include <benchmark.h>

static unsigned int FULL;

static unsigned int dfs(unsigned int row, unsigned int ld, unsigned int rd) {
  // Print the current state before diving into DFS
  printf("dfs(row=0x%x, ld=0x%x, rd=0x%x) -> FULL=0x%x\n", row, ld, rd, FULL);
  
  if (row == FULL) {
    return 1;
  } else {
    unsigned int pos = FULL & (~(row | ld | rd)), ans = 0;
    
    // Print available positions (pos) before trying
    printf("  Available positions: pos=0x%x\n", pos);
    
    while (pos) {
      unsigned int p = (pos & (~pos + 1));
      pos -= p;

      // Print the position being tried
      printf("  Trying position p=0x%x (row=0x%x, ld=0x%x, rd=0x%x)\n", p, row, ld, rd);
      
      ans += dfs(row | p, (ld | p) << 1, (rd | p) >> 1);
    }

    return ans;
  }
}

static unsigned int ans;

void bench_queen_prepare() {
  ans = 0;
  FULL = (1 << setting->size) - 1;
  printf("bench_queen_prepare: FULL=0x%x\n", FULL);  // Print the value of FULL for debugging
}

void bench_queen_run() {
  printf("Running DFS...\n");
  ans = dfs(0, 0, 0);
  printf("Completed DFS. Result: %u\n", ans);  // Print the result of DFS
}

int bench_queen_validate() {
  printf("Validating result: expected checksum=0x%x, computed ans=%u\n", setting->checksum, ans);
  return ans == setting->checksum;
}
