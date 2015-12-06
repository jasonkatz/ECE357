    .global _start

    .text

_start:
    mov     $4, %eax
    mov     $1, %ebx
    mov     $msg, %ecx
    mov     $13, %edx
    int     $0x80

msg:
    .ascii "Hello, world\n"
