.code

; quicker than NtReadVirtualMemory & NtWriteVirtualMemory slightly.... no touchy this

Driver_ReadVirtualMemory PROC
	mov r10, rcx
	mov eax, 63
	syscall
	ret
Driver_ReadVirtualMemory ENDP

Driver_WriteVirtualMemory PROC
	mov r10, rcx
	mov eax, 58
	syscall
	ret
Driver_WriteVirtualMemory ENDP

Driver_WriteMousePosition PROC
	; rcx = process handle (unused, uses current process)
	; rdx = base address of input object
	; r8  = new MousePosition Vector2 (float x, float y)
	mov r10, rcx
	mov rax, rdx          ; base = input object address
	mov [rax + 0ech], r8d ; write lower 4 bytes (X)
	mov [rax + 0f0h], r9d  ; write upper 4 bytes (Y) - Vector2 is [x:0, y:4]
	ret
Driver_WriteMousePosition ENDP

END