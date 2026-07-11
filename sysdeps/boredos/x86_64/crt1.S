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
    # At entry, %rdi = argc, %rsi = argv.
    # We need to call: __mlibc_entry(main, argc, argv)
    # Register mapping for __mlibc_entry:
    # %rdi: main function pointer
    # %rsi: argc
    # %rdx: argv

    mov rdx, rsi                  # argv -> 3rd argument
    mov rsi, rdi                  # argc -> 2nd argument
    lea rdi, main[rip]            # main -> 1st argument

    # Align the stack to 16 bytes for System V ABI compatibility
    and rsp, -16

    call __mlibc_entry

.section .note.GNU-stack,"",%progbits

.att_syntax prefix
