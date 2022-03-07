;[]-----------------------------------------------------------------[]
;|      H_LDIV.ASM -- long division routine                          |
;[]-----------------------------------------------------------------[]
.model medium
.386C   ;JAB - we use 386 instructions

_TEXT   segment public byte 'CODE'
        assume  cs:_TEXT
        public  __aNldiv
__aNldiv:

        push bp
        mov     bp,sp   ;Save BP, and set it equal to stack
        push dx
        mov     eax,dword ptr [bp+4]
        cdq
        div dword ptr [bp+8]
        cmp eax,0FFFFh
        jb norm_period
        mov ax,0FFFEh
norm_period:

        pop dx
        pop     bp              ;Restore BP
        retn           ;Return to original caller

_TEXT   ends
        end
