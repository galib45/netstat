.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits

.text  
    .global syscall5     

    syscall5:
        mov rax, rdi
        mov rdi, rsi
        mov rsi, rdx
        mov rdx, rcx
        mov r10, r8
        mov r8, r9
        syscall
        ret

	.global _start
	_start:
	  xor rbp,rbp
	  xor r9,r9
	  pop rdi     /* argc */
	  mov rsi,rsp /* argv */
	  call main
	  call myexit
