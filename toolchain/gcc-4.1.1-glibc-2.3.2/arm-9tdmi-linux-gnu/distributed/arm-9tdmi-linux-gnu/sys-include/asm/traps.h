#ifndef _ASMARM_TRAP_H
#define _ASMARM_TRAP_H


struct undef_hook {
	struct list_head node;
	__u32 instr_mask;
	__u32 instr_val;
	__u32 cpsr_mask;
	__u32 cpsr_val;
	int (*fn)(struct pt_regs *regs, unsigned int instr);
};

void register_undef_hook(struct undef_hook *hook);
void unregister_undef_hook(struct undef_hook *hook);

#endif
