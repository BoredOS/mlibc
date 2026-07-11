.intel_syntax noprefix

.global syscall

syscall:
    push rbp
    mov rbp, rsp

    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8, r9
    mov r9, [rbp+16]

    syscall

    leave
    ret

.hidden syscall

.att_syntax prefix

#include <mlibc-asm/helpers.h>
GNU_STACK_NOTE()
