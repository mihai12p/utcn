
%include "./kernel/Utils.inc"

BYTES_IN_SECTOR                    equ 512
SECTORS_IN_HEAD                    equ 18
HEADS_IN_CYLINDER                  equ 2
CYLINDER_MBR_SSL                   equ 1
TOTAL_KERNEL_CYLINDERS             equ 6
LAST_KERNEL_CYLINDER               equ CYLINDER_MBR_SSL + TOTAL_KERNEL_CYLINDERS
DMA_BOUNDARY_ALERT                 equ 0xDC00 ; 0xDC00 + 0x2400 == 0x10000 and DMA gives boundary error

segment .text

[org 0x7E00]
[BITS 16]
SSL:
    mov  bx,    0x1000                 ; memory 0x9200 - 0x9FFFF is unused
    mov  es,    bx
    xor  bx,    bx                     ; read to address ES:BX == 0x1000

    mov  ch,    0x01                   ; start reading cylinder 1 (0 was for MBR, SSL)
    mov  cl,    0x01                   ; always read from sector 1
.read_next_cylinder:
    mov  dh,    0x00                   ; start reading the first head

    .read_next_head:
        cmp  bx,    DMA_BOUNDARY_ALERT
        jb   .read
        mov  bx,    es
        add  bx,    0x1000
        mov  es,    bx
        xor  bx,    bx                     ; ES:DC00h -> ES+1000h:0000h
    .read:
        mov  ah,    0x02                   ; read function
        mov  al,    SECTORS_IN_HEAD        ; read all the sectors from a head
        int  13h
        jc   .error

        add  bx,    SECTORS_IN_HEAD * BYTES_IN_SECTOR

        inc  dh                         ; go to next head
        cmp  dh,    HEADS_IN_CYLINDER
        jb   .read_next_head

    inc  ch                             ; go to next cylinder
    cmp  ch,    LAST_KERNEL_CYLINDER
    jb   .read_next_cylinder
    je   .success

.error:
    cli                    ; we should reset drive and retry, but we hlt
    hlt

.success:
    cli                    ; starting RM to PM32 transition
    lgdt [GDT]
    ENABLE_CR_FLAG CR0, CR0.PE
    jmp  24:.bits32        ; change the CS to 24 (index of FLAT_DESCRIPTOR_CODE32 entry)

[BITS 32]
.bits32:
    mov  ax,    32       ; index of FLAT_DESCRIPTOR_DATA32 entry
    mov  ds,    ax
    mov  es,    ax
    mov  gs,    ax
    mov  ss,    ax
    mov  fs,    ax

    cld
    mov  edi,    0x200000                     ; destination
    mov  esi,    0x10000                      ; initial source
    xor  ebx,    ebx
    mov  ecx,    TOTAL_KERNEL_CYLINDERS
.copy_next_cylinder:                       ; for ecx in range(TOTAL_KERNEL_CYLINDERS)
    push ecx                               ; save ecx
    mov  ecx,    HEADS_IN_CYLINDER
    .copy_next_head:                       ; for ecx in range(HEADS_IN_CYLINDER)
        push ecx                           ; save ecx
        cmp  ebx,    DMA_BOUNDARY_ALERT    ; do we have to increase esi?
        jb   .copy
        add  esi,    0x10000               ; increase esi
        xor  ebx,    ebx
    .copy:
        push esi                           ; save initial source
        add  esi,    ebx                      ; initial source += offset
        mov  ecx,    SECTORS_IN_HEAD * BYTES_IN_SECTOR
        rep  movsb                         ; copy
        add  ebx,    SECTORS_IN_HEAD * BYTES_IN_SECTOR
        pop  esi                           ; restore initial source
        pop  ecx                           ; restore ecx
        loop .copy_next_head
    pop  ecx                               ; restore ecx
    loop .copy_next_cylinder
    
    mov  [ds:0xb8000],  byte 'O'
    mov  [ds:0xb8002],  byte 'K'

is_A20_on?:
    pushad
    mov  edi,   0x100000
    mov  esi,   0x000000

    ; making sure that both addresses contain diffrent values.
    mov  [esi],     byte 'A'
    mov  [edi],     byte 'B'
    cmpsb             ; compare addresses to see if the're equivalent.
    popad
    jne A20_on        ; if not equivalent, A20 line is set.

    cli               ; if equivalent, the A20 line is cleared.
    hlt

A20_on:
    mov  eax,   0x201000    ; the hardcoded ASMEntryPoint of the Kernel
    call eax

    ;
    ; these instructions should not be reached
    ;
    cli
    hlt

segment .data

GDT:
    .limit  dw  GDTTable.end - GDTTable - 1
    .base   dd  GDTTable

GDTTable:
    ;                                        Index
    .null     dq 0                         ;  0
    .code64   dq FLAT_DESCRIPTOR_CODE64    ;  8
    .data64   dq FLAT_DESCRIPTOR_DATA64    ; 16
    .code32   dq FLAT_DESCRIPTOR_CODE32    ; 24
    .data32   dq FLAT_DESCRIPTOR_DATA32    ; 32
    .code16   dq FLAT_DESCRIPTOR_CODE16    ; 40
    .data16   dq FLAT_DESCRIPTOR_DATA16    ; 48
    .end:
