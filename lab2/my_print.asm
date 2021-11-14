global myputs
; void myputs(int fd, char *str, size_t len)
myputs:
  ; syscall(SYS_write, fd, str, len)
  mov rax, 1
  syscall

  ret
