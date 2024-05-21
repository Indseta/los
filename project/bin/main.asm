bits 64
default rel

extern printf

segment .data
	ca7c8193 db "%u", 0
	c30fa8d6 db 0xd, 0xa, 0
	c9b2a13eb db "%llu", 0
	ca7c3942 db "%d", 0
	c9b29cb9a db "%lld", 0
	c4ed94d95 db "hello world", 0
	c1592b1ed db "hello", 0

segment .bss

segment .text
	global f8f753e97
	global f7e1d10c6
	global f7e1d1104
	global f7e1d1169
	global fbe80efc2
	global f8e9ee751
	global f8e9ee78f
	global f8e9ee7f4
	global f7a3bd7d6
	global main

exit:
	leave
	ret
f8f753e97:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov sil, [rbp + 16]
	mov rdx, rsi
	lea rcx, [ca7c8193]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f7e1d10c6:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov si, [rbp + 16]
	mov rdx, rsi
	lea rcx, [ca7c8193]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f7e1d1104:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov esi, [rbp + 16]
	mov rdx, rsi
	lea rcx, [ca7c8193]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f7e1d1169:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov rsi, [rbp + 16]
	mov rdx, rsi
	lea rcx, [c9b2a13eb]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
fbe80efc2:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov sil, [rbp + 16]
	movsx rdx, sil
	lea rcx, [ca7c3942]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f8e9ee751:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov si, [rbp + 16]
	movsx rdx, si
	lea rcx, [ca7c3942]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f8e9ee78f:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov esi, [rbp + 16]
	movsx rdx, esi
	lea rcx, [ca7c3942]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f8e9ee7f4:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov rsi, [rbp + 16]
	mov rdx, rsi
	lea rcx, [c9b29cb9a]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
f7a3bd7d6:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov rcx, [rbp + 16]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	jmp exit
main:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	sub rsp, 16
	mov si, 1
	neg si
	mov word [rsp + 0], si
	call f7e1d10c6
	add rsp, 16
	lea rcx, [c4ed94d95]
	call printf
	lea rcx, [c1592b1ed]
	call printf
	lea rcx, [c30fa8d6]
	call printf
	xor rax, rax
	jmp exit
