.global _start

_start:
    movq    $1, %rax        # Move 1 (write) into %rax register
    movq    $1, %rdi        # Pass 1 (stdout) to write
    movq    $message, %rsi  # Pass the message to write
    movq    $14, %rdx       # Pass 14 (length of string to write
    syscall                 # write(1, "Hello, world!\n", 14);

    movq    $60, %rax       # Move 60 (exit) into %rax register
    movq    $0, %rdi        # Pass 0 (status) to exit
    syscall                 # exit(0);

message:
    .ascii "Hello, world!\n"
