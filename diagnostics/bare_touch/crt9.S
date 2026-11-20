.section .crt0,"ax"
.arm
.global _start
_start:
 ldr r0,=0x04000208
 mov r1,#0
 str r1,[r0]
 msr cpsr_c,#0xD3
 ldr sp,=0x0237F000
 msr cpsr_c,#0xDF
 ldr sp,=0x0237E000
 ldr r0,=__bss_start
 ldr r1,=__bss_end
 mov r2,#0
1: cmp r0,r1
 strlo r2,[r0],#4
 blo 1b
 bl main
2: b 2b
.pool
