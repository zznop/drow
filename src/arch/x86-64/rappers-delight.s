.intel_syntax noprefix

jmp past

message:
    .ascii "See, I am drow, and I'd like to say hello,\n"
    .ascii "To the black, to the white, the red and the brown,\n"
    .ascii "The purple and yellow. But first, I gotta\n"
    .ascii "Bang bang, the boogie to the boogie,\n"
    .ascii "Say up jump the boogie to the bang bang boogie,\n"
    .ascii "Let's rock, you don't stop ...\n\n"

past:
    mov rdi, 1                    /* STDOUT file descriptor */
    lea rsi, [rip + message]      /* Pointer to message string */
    mov rdx, 253                  /* Message size */
    mov rax, 1                    /* Write syscall number */
    syscall                       /* Execute system call */
    ret                           /* Return back to caller */
