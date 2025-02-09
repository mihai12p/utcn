
%include "Utils.inc"

IMPORTFROMC KernelMain, InterruptHandler
EXPORT2C ASMEntryPoint, __magic, gISR_stub_table, PML4

segment .text

[BITS 32]
ASMEntryPoint:
    cli
    mov  DWORD [000B8000h],    'O1S1'
%ifidn __OUTPUT_FORMAT__, win32
    mov  DWORD [000B8004h],    '3121'                   ; 32 bit build marker
%else
    mov  DWORD [000B8004h],    '6141'                   ; 64 bit build marker
%endif

    mov  esp,    TOP_OF_STACK                           ; just below the kernel

    ;
    ; CR0.PG is cleared at this moment
    ;

    ;
    ; enable physical-address extension by setting CR4.PAE
    ;
    ENABLE_CR_FLAG CR4, CR4.PAE

    ;
    ; load CR3 with the physical base address of PML4
    ;
    lea  eax,   [PML4]
    mov  cr3,   eax

    ;
    ; enable IE-32e (long) mode by setting IA32_EFER.LME
    ;
    mov  ecx,   IA32_EFER_MSR
    rdmsr
    or   eax,   IA32_EFER_MSR.LME
    wrmsr

    ;
    ; enable paging by setting CR0.PG
    ;
    ENABLE_CR_FLAG CR0, CR0.PG

    ;
    ; enable SSE
    ;
    ENABLE_CR_FLAG CR4, CR4.OSFXSR

    jmp  8:.bits64           ; change the CS to 8 (index of FLAT_DESCRIPTOR_CODE64 entry)

[BITS 64]
.bits64:
    mov  ax,    16       ; index of FLAT_DESCRIPTOR_DATA64 entry
    mov  ds,    ax
    mov  es,    ax
    mov  gs,    ax
    mov  ss,    ax
    mov  fs,    ax

    call __invalidate_TLB

    mov  rax,   KernelMain
    call rax

    break
    cli
    hlt

.interrupts:
    ISR_NOERRCODE 0
    ISR_NOERRCODE 1
    ISR_NOERRCODE 2
    ISR_NOERRCODE 3
    ISR_NOERRCODE 4
    ISR_NOERRCODE 5
    ISR_NOERRCODE 6
    ISR_NOERRCODE 7
    ISR_ERRCODE   8
    ISR_NOERRCODE 9
    ISR_ERRCODE   10
    ISR_ERRCODE   11
    ISR_ERRCODE   12
    ISR_ERRCODE   13
    ISR_ERRCODE   14
    ISR_NOERRCODE 15
    ISR_NOERRCODE 16
    ISR_ERRCODE   17
    ISR_NOERRCODE 18
    ISR_NOERRCODE 19
    ISR_NOERRCODE 20
    ISR_NOERRCODE 21
    ISR_NOERRCODE 22
    ISR_NOERRCODE 23
    ISR_NOERRCODE 24
    ISR_NOERRCODE 25
    ISR_NOERRCODE 26
    ISR_NOERRCODE 27
    ISR_NOERRCODE 28
    ISR_NOERRCODE 29
    ISR_ERRCODE   30
    ISR_NOERRCODE 31
    ISR_NOERRCODE 32    ; Timer
    ISR_NOERRCODE 33    ; Keyboard
%assign i 34
%rep    46
    ISR_NOERRCODE i
    %assign i i+1
%endrep
    ISR_NOERRCODE 80    ; IPI_T1
    ISR_NOERRCODE 81    ; IPI_T2
    ISR_NOERRCODE 82    ; LAPIC_TICK
%assign i 83
%rep    173
    ISR_NOERRCODE i
    %assign i i+1
%endrep
__isr_common_stub:
; Stack:
;     [rsp + 30h]     SS
;     [rsp + 28h]     RSP
;     [rsp + 20h]     RFLAGS
;     [rsp + 18h]     CS
;     [rsp + 10h]     RIP
;     [rsp + 08h]     Error code
;     [rsp + 00h]     Interrupt index

    sub  rsp, CPUContext_size

    mov  [rsp + CPUContext.rax], rax
    mov  [rsp + CPUContext.rbx], rbx
    mov  [rsp + CPUContext.rcx], rcx
    mov  [rsp + CPUContext.rdx], rdx
    mov  [rsp + CPUContext.rsi], rsi
    mov  [rsp + CPUContext.rdi], rdi
    mov  rcx, [rsp + CPUContext_size + 28h]
    mov  [rsp + CPUContext.rsp], rcx
    mov  [rsp + CPUContext.rbp], rbp
    mov  [rsp + CPUContext.r8], r8
    mov  [rsp + CPUContext.r9], r9
    mov  [rsp + CPUContext.r10], r10
    mov  [rsp + CPUContext.r11], r11
    mov  [rsp + CPUContext.r12], r12
    mov  [rsp + CPUContext.r13], r13
    mov  [rsp + CPUContext.r14], r14
    mov  [rsp + CPUContext.r15], r15
    mov  rcx, [rsp + CPUContext_size + 10h]
    mov  [rsp + CPUContext.rip], rcx
    mov  rcx, [rsp + CPUContext_size + 18h]
    mov  [rsp + CPUContext.cs], cx
    mov  rcx, [rsp + CPUContext_size + 30h]
    mov  [rsp + CPUContext.ss], cx
    mov  [rsp + CPUContext.ds], ds
    mov  [rsp + CPUContext.es], es
    mov  [rsp + CPUContext.fs], fs
    mov  [rsp + CPUContext.gs], gs
    mov  rcx, [rsp + CPUContext_size + 20h]
    mov  [rsp + CPUContext.rflags], ecx
    mov  rcx, cr0
    mov  [rsp + CPUContext.cr0], rcx
    mov  rcx, cr2
    mov  [rsp + CPUContext.cr2], rcx
    mov  rcx, cr3
    mov  [rsp + CPUContext.cr3], rcx
    mov  rcx, cr4
    mov  [rsp + CPUContext.cr4], rcx
    mov  rcx, dr0
    mov  [rsp + CPUContext.dr0], rcx
    mov  rcx, dr1
    mov  [rsp + CPUContext.dr1], rcx
    mov  rcx, dr2
    mov  [rsp + CPUContext.dr2], rcx
    mov  rcx, dr3
    mov  [rsp + CPUContext.dr3], rcx
    mov  rcx, dr6
    mov  [rsp + CPUContext.dr6], rcx
    mov  rcx, dr7
    mov  [rsp + CPUContext.dr7], rcx

    mov  rcx, rsp
    mov  rdx, [rsp + CPUContext_size + 00h]
    mov  r8,  [rsp + CPUContext_size + 08h]
    xor  r9,  r9

    call InterruptHandler

    mov  rax, [rsp + CPUContext.rsp]
    mov  [rsp + CPUContext_size + 28h], rax
    mov  rax, [rsp + CPUContext.rip]
    mov  [rsp + CPUContext_size + 10h], rax
    xor  rax, rax
    mov  ax,  [rsp + CPUContext.cs]
    mov  [rsp + CPUContext_size + 18h], rax
    xor  rax, rax
    mov  ax,  [rsp + CPUContext.ss]
    mov  [rsp + CPUContext_size + 30h], rax
    xor  rax, rax
    mov  eax, [rsp + CPUContext.rflags]
    mov  [rsp + CPUContext_size + 20h], rax
    mov  rax, [rsp + CPUContext.rax]
    mov  rbx, [rsp + CPUContext.rbx]
    mov  rcx, [rsp + CPUContext.rcx]
    mov  rdx, [rsp + CPUContext.rdx]
    mov  rsi, [rsp + CPUContext.rsi]
    mov  rdi, [rsp + CPUContext.rdi]
    mov  r8,  [rsp + CPUContext.r8]
    mov  r9,  [rsp + CPUContext.r9]
    mov  r10, [rsp + CPUContext.r10]
    mov  r11, [rsp + CPUContext.r11]
    mov  r12, [rsp + CPUContext.r12]
    mov  r13, [rsp + CPUContext.r13]
    mov  r14, [rsp + CPUContext.r14]
    mov  r15, [rsp + CPUContext.r15]
    mov  rbp, [rsp + CPUContext.rbp]
    mov  ds,  [rsp + CPUContext.ds]
    mov  es,  [rsp + CPUContext.es]
    mov  fs,  [rsp + CPUContext.fs]
    mov  gs,  [rsp + CPUContext.gs]

    add  rsp, CPUContext_size + 10h

    iretq

__magic:
    xchg bx,    bx
    ret

__invalidate_TLB:
    ;
    ; invalidate all entries in the TLB (Translation Lookaside Buffer)
    ; where the processor may cache information about the translation of linear addresses
    ;
    mov  rax,    cr3
    mov  cr3,    rax
    ret

segment .data

;
; each page table contains 512 entries
; each entry in a page table maps a single 4KB page
; total memory mapped by a page table equals to 2MB
;
_page_table:
    DEFINE_PT PT0,  0000000000000000h        ; 000000 - 1FFFFF
    DEFINE_PT PT1,  0000000000200000h        ; 200000 - 3FFFFF
    DEFINE_PT PT2,  0000000000400000h        ; 400000 - 5FFFFF
    DEFINE_PT PT3,  0000000000600000h        ; 600000 - 7FFFFF
    DEFINE_PT PT4,  0000000000800000h        ; 800000 - 9FFFFF
    DEFINE_PT PT20, 0000000000000000h        ; 000000 - 1FFFFF
    DEFINE_PT PT21, 0000000000200000h        ; 200000 - 3FFFFF
    DEFINE_PT PT22, 0000000000400000h        ; 400000 - 5FFFFF
    DEFINE_PT PT23, 0000000000600000h        ; 600000 - 7FFFFF
    DEFINE_PT PT24, 0000000000800000h        ; 800000 - 9FFFFF

;
; each entry covers 2MB
;
_page_directory:
    align PAGE_SIZE
    PD0:
        dq PT0 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT1 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT2 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT3 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT4 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT4 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT4 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT4 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT4 + (BIT_PRESENT | BIT_READ_WRITE)
        times 512 - ($ - PD0) / 8 dq 0

    align PAGE_SIZE
    PD2:
        dq PT20 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT21 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT22 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT23 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT24 + (BIT_PRESENT | BIT_READ_WRITE)
        times 512 - ($ - PD2) / 8 dq 0

;
; each entry covers 1GB
;
_page_directory_pointer_table:
    align PAGE_SIZE
    PDPT0:
        dq PD0 + (BIT_PRESENT | BIT_READ_WRITE)             ; PDE0
        times 512 - ($ - PDPT0) / 8 dq 0

    align PAGE_SIZE
    PDPT2:
        dq PD2 + (BIT_PRESENT | BIT_READ_WRITE)             ; PDE2
        times 512 - ($ - PDPT2) / 8 dq 0

;
; each entry covers 512GB
;
_page_map_level_4:
    align PAGE_SIZE
    PML4:
        dq PDPT0 + (BIT_PRESENT | BIT_READ_WRITE)           ; PDPTE0
        dq 0                                                ; PDPTE1 (unused)
        dq PDPT2 + (BIT_PRESENT | BIT_READ_WRITE)           ; PDPTE2
        times 512 - ($ - PML4) / 8 dq 0

; PML4
; |- PML4[0] -> PDPT0 (Identity Mapping)
; |   |- PDPT0[0] -> PD0
; |       |- PD0[0] -> PT0 (PA 0000000000000000h - 00000000001FFFFFh -> VA 0000000000000000h - 00000000001FFFFFh)
; |       |- PD0[1] -> PT1 (PA 0000000000200000h - 00000000003FFFFFh -> VA 0000000000200000h - 00000000003FFFFFh)
; |       |- PD0[2] -> PT2 (PA 0000000000400000h - 00000000005FFFFFh -> VA 0000000000400000h - 00000000005FFFFFh)
; |       |- PD0[3] -> PT3 (PA 0000000000600000h - 00000000007FFFFFh -> VA 0000000000600000h - 00000000007FFFFFh)
; |       |- PD0[4] -> PT4 (PA 0000000000800000h - 00000000009FFFFFh -> VA 0000000000800000h - 00000000009FFFFFh)
; |- PML4[1] -> Unused
; |- PML4[2] -> PDPT2 (Non-Identity Mapping)
; |   |- PDPT2[0] -> PD2
; |       |- PD2[0] -> PT20 (PA 0000000000000000h - 00000000001FFFFFh -> VA 0000010000000000h - 00000100001FFFFFh)
; |       |- PD2[1] -> PT21 (PA 0000000000200000h - 00000000003FFFFFh -> VA 0000010000200000h - 00000100003FFFFFh)
; |       |- PD2[2] -> PT22 (PA 0000000000400000h - 00000000005FFFFFh -> VA 0000010000400000h - 00000100005FFFFFh)
; |       |- PD2[3] -> PT23 (PA 0000000000600000h - 00000000007FFFFFh -> VA 0000010000600000h - 00000100007FFFFFh)
; |       |- PD2[4] -> PT24 (PA 0000000000800000h - 00000000009FFFFFh -> VA 0000010000800000h - 00000100009FFFFFh)
; |- PML4[3-511] -> Unused

gISR_stub_table:
%assign i 0
%rep    256
    dq isr_stub_%+i
    %assign i i+1
%endrep

segment .bss

align 16
struc CPUContext
    .rax    resq 1
    .rbx    resq 1
    .rcx    resq 1
    .rdx    resq 1
    .rsi    resq 1
    .rdi    resq 1
    .rsp    resq 1
    .rbp    resq 1
    .r8     resq 1
    .r9     resq 1
    .r10    resq 1
    .r11    resq 1
    .r12    resq 1
    .r13    resq 1
    .r14    resq 1
    .r15    resq 1
    .rip    resq 1
    .cs     resw 1
    .ss     resw 1
    .ds     resw 1
    .es     resw 1
    .fs     resw 1
    .gs     resw 1
    .rflags resd 1
    .cr0    resq 1
    .cr2    resq 1
    .cr3    resq 1
    .cr4    resq 1
    .dr0    resq 1
    .dr1    resq 1
    .dr2    resq 1
    .dr3    resq 1
    .dr6    resq 1
    .dr7    resq 1
endstruc
