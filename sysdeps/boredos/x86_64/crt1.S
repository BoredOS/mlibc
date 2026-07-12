.intel_syntax noprefix

.section .text
.global _start
.weak main
.global __mlibc_entry

.global __dso_handle
.hidden __dso_handle
.weak __dso_handle
__dso_handle:
    .quad 0

_start:
    # At entry, rsp points to the System V ABI stack frame.
    # We need to call: __mlibc_entry(rsp, main)
    # Register mapping for __mlibc_entry:
    # %rdi: entry_stack (rsp)
    # %rsi: main function pointer

    mov rdi, rsp                  # rsp -> 1st argument (entry_stack)
    lea rsi, main[rip]            # main -> 2nd argument

    # Align the stack to 16 bytes for System V ABI compatibility
    and rsp, -16

    call __mlibc_entry

.section .note.GNU-stack,"",%progbits

.att_syntax prefix
