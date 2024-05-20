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
	mov edx, 0
	mov dword [rbp - 0], edx
	jmp .wlc0
.wle0:
	xor rax, rax
	jmp exit
.wlc0:
	mov eax, [rbp - 0]
	mov ebx, 10
	cmp eax, ebx
	setl al
	mov ecx, eax
	cmp cl, 1
	je .wlm0
	jne .wle0
.wlm0:
	sub rsp, 16
	mov esi, [rbp - 0]
	movsx rdx, esi
	lea rcx, [ca7c3942]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	mov eax, [rbp - 0]
	mov ebx, 1
	add eax, ebx
	mov edx, eax
	mov dword [rbp - 0], edx
	add rsp, 16
	jmp .wlc0
