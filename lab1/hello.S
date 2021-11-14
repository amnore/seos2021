org 7c00h
  jmp hello
msg: db "Hello World!"
hello:
  mov ax, 03h
  int 10h
  mov bp, msg
  mov cx, (hello - msg)
  mov ah, 13h
  mov bx, 000ch
  mov dl, 0
  int 10h
  jmp $
times 510 - ($-$$) db 0
dw 0xaa55
