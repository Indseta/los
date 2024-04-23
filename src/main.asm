bits 64
default rel

extern ExitProcess
extern printf

segment .data
	msg6ef9066f db "%d", 0xd, 0xa, 0

segment .text
	global main

main:
	push rbp
	mov rbp, rsp
	sub rsp, 32

	mov edx, 2763
	lea rcx, [msg6ef9066f]
	call printf

	xor rax, rax
	call ExitProcess
