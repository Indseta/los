bits 64
default rel

extern printf

segment .data
	ca7c3942 db "%d", 0
	c30fa8d6 db 0xd, 0xa, 0

segment .bss

segment .text
	global main

exit:
	leave
	ret
main:
	push rbp
	mov rbp, rsp
	sub rsp, 48
	mov edx, 100
	mov dword [rbp - 0], edx
	mov eax, [rbp - 0]
	mov ebx, 100
	cmp eax, ebx
	sete al
	mov ecx, eax
	cmp cl, 1
	je .cndm0
.cnde0:
	xor rax, rax
	jmp exit
.cndm0:
	sub rsp, 16
	mov edx, 20000
	mov dword [rbp - 4], edx
	mov esi, [rbp - 4]
	movsx rdx, esi
	lea rcx, [ca7c3942]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	add rsp, 16
	jmp .cnde0
