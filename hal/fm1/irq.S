/* SPDX-License-Identifier: GPL-3.0-only
 * The timer's interrupt entry (irq.c): what a C call may change is kept (r0-r3, the status, the return addresses),
 * the C body called, all put back, csync, rti (the compiler does not give the csync an rti needs, so no ISR is C
 * itself). Written for this project; the frame as the FM-1's other firmware keeps it. */
	.section .text.isr_tick,"ax",@progbits
	.globl isr_tick
	.balign 2
isr_tick:
	[--sp] = {psr, rets, reti}
	[--sp] = {r3-r0}
	call irq_tick
	{r3-r0} = [sp++]
	{psr, rets, reti} = [sp++]
	csync
	rti
