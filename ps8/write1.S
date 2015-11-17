.global _start

_start:
    movq    $1, %rax        # Move 1 (write) into %rax register
    movq    $1, %rdi        # Pass 1 (stdout) as 1st param to write
    movq    $message, %rsi  # Pass the message to write
    movq    $14, %rdx       # Pass 14 (length of string to write
    syscall

message:
    .ascii "Hello, world!\n"
