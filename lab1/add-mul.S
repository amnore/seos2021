section .rodata
negsym: db "-"
newline: db `\n`


section .bss
buf: resb 100
cneg: resb 1
bneg: resb 1
aneg: resb 1
c: resb 43
b: resb 22
a: resb 22


section .text
global _start
_start:
  call main
; syscall(SYS_exit, 0)
  mov rax, 0xe7
  mov rdi, 0
  syscall


; void rev(char *s)
rev: 
; int i=0;
; while(s[i]) i++;
  mov rcx, 0
rev_find_tail:
  inc rcx
  cmp BYTE [rdi+rcx], 0
  jne rev_find_tail

; i--;
  dec rcx

; for(int j=0;j<i;j++,i--) swap(s[i],s[j]);
  mov rdx, 0
rev_swap:
  mov sil, [rdi+rcx]
  mov r8b, [rdi+rdx]
  mov [rdi+rdx], sil
  mov [rdi+rcx], r8b
  inc rdx
  dec rcx
  cmp rdx, rcx
  jl rev_swap
  ret 


; void pre(char *s)
pre:
  call rev

; for(;*s;s++) *s-='0';
pre_loop:
  sub BYTE [rdi], 48
  inc rdi
  cmp BYTE [rdi], 0
  jne pre_loop
  ret 


; void post()
post:
; char *p=c+sizeof(c)-1;
  lea rax, c+42
  lea rdi, c
post_find_first_nonzero:
; while(p>c&&*p=='\0') p--;
  cmp rax, rdi
  jle post_check_zero
  cmp BYTE [rax], 0
  jne post_check_zero
  dec rax
  jmp post_find_first_nonzero

post_check_zero:
; if(p==c&&*p=='\0') cneg=false;
  cmp rax, rdi
  jne post_fix_c
  cmp BYTE [c], 0
  jne post_fix_c
  mov BYTE [cneg], 0

; for(;p>=c;p--) *p+='0';
post_fix_c:
  add BYTE [rax], 48
  dec rax
  cmp rax, rdi
  jge post_fix_c

  jmp rev


; void add()
add:
  mov r9b, [aneg]
  mov r10b, [bneg]
  mov rsi, 0

; for(int i=0;i<sizeof(a)-1;i++)
add_loop1:
; c[i]+=(aneg?-a[i]:a[i])+(bneg?-b[i]+b[i])
  mov cl, [c+rsi]
  mov r8b, [a+rsi]
  test r9b, r9b
  je add_bneg
  neg r8d

add_bneg:
  mov dil, [b+rsi]
  test r10b, r10b
  je add_add_to_c
  neg edi

add_add_to_c:
  add ecx, r8d
  add ecx, edi

; if(c[i]>9) {c[i]-=10;c[i+1]++;}
  cmp cl, 9
  jg add_c_gt_9
  mov [c+rsi], cl
  jmp add_chk_c_lt_0

add_c_gt_9:
  sub ecx, 10
  inc BYTE [c+rsi+1]
  mov [c+rsi], cl

add_chk_c_lt_0:
; if(c[i]<0) {c[i]+=10;c[i+1]--;}
  mov cl, [c+rsi]
  test cl, cl
  jns add_loop1_end
  add ecx, 10
  dec BYTE [c+rsi+1]
  mov [c+rsi], cl

add_loop1_end:
  inc rsi
  cmp rsi, 21
  jne add_loop1

; if (c[sizeof(a) - 1] < 0)
  cmp BYTE [c+21], 0
  jns add_finish
  mov BYTE [cneg], 1
  dec BYTE [c]

  lea rdx, c
  mov dil, 9
  lea rsi, c+22
add_loop_fix_c:
  mov al, [rdx]
  mov ecx, edi
  sub ecx, eax

; if (c[i]>9) {c[i]-=10;c[i+1]++;}
  cmp cl, 9
  jg add_loop_fix_c_inc
  mov BYTE [rdx], cl
  jmp add_loop_fix_c_finish

add_loop_fix_c_inc:
  not eax
  inc BYTE [rdx+1]
  mov BYTE [rdx], al

add_loop_fix_c_finish:
  inc rdx
  cmp rsi, rdx
  jne add_loop_fix_c

  mov BYTE [c+22], 0

add_finish:
  ret 


; void mul()
mul:
; cneg=aneg^bneg;
  mov al, [aneg]
  xor al, [bneg]
  mov [cneg], al

  mov r8b, 10
  mov rsi, 0
mul_loopi:
; for(int i=0;i<sizeof(a)-1;i++)
  mov r10b, [a+rsi]
  lea rcx, [c+rsi]

  mov rdi, 0
mul_loopj:
; for(int j=0;j<sizeof(b)-1;j++)
; c[i+j]+=a[i]*b[j];
  mov al, [b+rdi]
  imul eax, r10d
  add [rcx], al

; for(int k=i+j;c[k]>9;k++)
  mov rdx, rcx
mul_loopk:
  movsx ax, BYTE [rdx]
  cmp al, 9
  jle mul_loopj_end
; int t=c[k]/10; c[k]%=10;c[k+1]+=t;
  idiv r8b
  add [rdx+1], al
  mov [rdx], ah
  inc rdx
  jmp mul_loopk

mul_loopj_end:
  inc rdi
  inc rcx
  cmp rdi, 21
  jne mul_loopj

  inc rsi
  cmp rsi, 21
  jne mul_loopi

  ret 

; char *copy(char *dst, bool *sig, char *src)
copy:
; if(*src=='-') {*sig=true;src++;}
  cmp BYTE [rdx], 45
  jne copy_loop
  mov BYTE [rsi], 1
  inc rdx

copy_loop:
; for(;*src!='\n'&&*src!=' ';src++,dst++) *dst=*src;
  mov cl, [rdx]
  cmp cl, 10
  je copy_finish
  cmp cl, 32
  je copy_finish
  mov [rdi], cl
  inc rdi
  inc rdx
  jmp copy_loop

copy_finish:
; return src;
  mov rax, rdx
  ret 

; void clear()
clear:
;for(int i=0;i<sizeof(c);i++) c[i]='\0';
  mov eax, 0
  mov rcx, 43
  lea rdi, c
  rep stosb

;cneg=false
  mov BYTE [cneg], 0
  ret 


scan:
; syscall(SYS_read, 0, buf, sizeof(buf));
  mov rax, 0
  mov rdi, 0
  lea rsi, buf
  mov rdx, 100
  syscall 
  ret 


print:
  mov rdi, 1
; if (cneg) syscall(SYS_write, 1, negsym, 1);
  cmp BYTE [cneg], 0
  je print_c
  mov rax, 1
  lea rsi, negsym
  mov rdx, 1
  syscall 

print_c:
; syscall(SYS_write, 1, c, sizeof(c));
  mov rax, 1
  lea rsi, c
  mov rdx, 43
  syscall 

; syscall(SYS_write, 1, newline, 1);
  mov rax, 1
  lea rsi, newline ; tmp99,
  mov rdx, 1
  syscall 

  ret 


global main
main:
  call scan

; char *sep=copy(a,&aneg,buf);
  lea rdi, a
  lea rsi, aneg
  lea rdx, buf ; tmp89,
  call copy

; copy(b,&bneg,sep+1);
  lea rdi, b
  lea rsi, bneg
  lea rdx, [rax+1]
  call copy

; pre(a);
  lea rdi, a
  call pre

; pre(b);
  lea rdi, b
  call pre

  call add
  call post
  call print
  call clear
  call mul
  call post
  call print
  mov eax, 0
  ret
