#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  cpu.idtr.limit=vaddr_read(id_dest->addr,2);
  if(decoding.is_operand_size_16)
    cpu.idtr.base=vaddr_read(id_dest->addr+2,4)& 0x00ffffff;
  else
    cpu.idtr.base=vaddr_read(id_dest->addr+2,4);
  

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  switch (id_dest->reg) {
    case 0:  // CR0
      cpu.CR0 = id_src->val;
      break;
    case 3:  // CR3
      cpu.CR3 = id_src->val;
      break;
    default:
      Log("Unsupported control register: CR%d", id_dest->reg);
      assert(0);
  }

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}


make_EHelper(mov_cr2r) {
  rtlreg_t t0;

  switch (id_src->reg) {
    case 0:  // CR0
      t0 = cpu.CR0;
      break;
    case 3:  // CR3
      t0 = cpu.CR3;
      break;
    default:
      Log("Unsupported control register: CR%d", id_src->reg);
      assert(0);
  }

  rtl_sr(id_dest->reg, 4, &t0);  // 写入 GPR

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();  // 跳过 QEMU 对比
#endif
}


extern void raise_intr(uint8_t NO, vaddr_t ret_addr);

make_EHelper(int) {

  uint8_t NO = id_dest -> val & 0xff;
  raise_intr(NO, decoding.seq_eip);
  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  // TODO();
  rtl_pop(&cpu.eip);
  rtl_pop(&cpu.cs);
  rtl_pop(&t0);
  memcpy(&cpu.eflags, &t0, sizeof(cpu.eflags));

  decoding.jmp_eip = 1;
  decoding.seq_eip = cpu.eip;


  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  rtl_li(&t0, pio_read(id_src->val, id_dest->width));
  operand_write(id_dest, &t0);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val, id_src->width, id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}