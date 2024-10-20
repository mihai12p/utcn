
%include "Utils.inc"

IMPORTFROMC KernelMain
EXPORT2C ASMEntryPoint, __cli, __sti, __magic, __enableSSE

segment .text

[BITS 32]
ASMEntryPoint:
    cli
    mov  DWORD [0x000B8000],    'O1S1'
%ifidn __OUTPUT_FORMAT__, win32
    mov  DWORD [0x000B8004],    '3121'                  ; 32 bit build marker
%else
    mov  DWORD [0x000B8004],    '6141'                  ; 64 bit build marker
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

    jmp  8:.bits64           ; change the CS to 8 (index of FLAT_DESCRIPTOR_CODE64 entry)

[BITS 64]
.bits64:
    mov  ax,    16       ; index of FLAT_DESCRIPTOR_DATA64 entry
    mov  ds,    ax
    mov  es,    ax
    mov  gs,    ax
    mov  ss,    ax
    mov  fs,    ax

    ;
    ; invalidate identity-mapping
    ;
    ; mov rax,  PML4
    ; mov QWORD [rax],  0

    ;
    ; invalidate all entries in the TLB (Translation Lookaside Buffers)
    ; where the processor may cache information about the translation of linear addresses
    ;
    mov rax,    cr3
    mov cr3,    rax

    mov  rax,   1
    shl  rax,   40
    add  rax,   KernelMain
    call rax

    break
    cli
    hlt

[BITS 32]
__cli:
    cli
    ret

__sti:
    sti
    ret

__magic:
    xchg bx,    bx
    ret

__enableSSE:                ; enable SSE instructions (CR4.OSFXSR = 1)  
    ENABLE_CR_FLAG CR4, CR4.OSFXSR
    ret

segment .data

;
; each page table contains 512 entries
; each entry in a page table maps a single 4KB page
; total memory mapped by a page table equals to 2MB
;
_page_table:
    DEFINE_PT PT0, 0x0000000000000000       ; 000000 - 1FFFFF
    DEFINE_PT PT1, 0x0000000000200000       ; 200000 - 3FFFFF
    DEFINE_PT PT2, 0x0000000000000000       ; 000000 - 1FFFFF
    DEFINE_PT PT3, 0x0000000000200000       ; 200000 - 3FFFFF

;
; each entry covers 2MB
;
_page_directory:
    align PAGE_SIZE
    PD0:
        dq PT0 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT1 + (BIT_PRESENT | BIT_READ_WRITE)
        times 512 - ($ - PD0) / 8 dq 0

    align PAGE_SIZE
    PD2:
        dq PT2 + (BIT_PRESENT | BIT_READ_WRITE)
        dq PT3 + (BIT_PRESENT | BIT_READ_WRITE)
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
; |       |- PD0[0] -> PT0 (PA 0x0000000000000000 - 0x00000000001FFFFF -> VA 0x0000000000000000 - 0x00000000001FFFFF)
; |       |- PD0[1] -> PT1 (PA 0x0000000000200000 - 0x00000000003FFFFF -> VA 0x0000000000200000 - 0x00000000003FFFFF)
; |- PML4[1] -> Unused
; |- PML4[2] -> PDPT2 (Non-Identity Mapping)
; |   |- PDPT2[0] -> PD2
; |       |- PD2[0] -> PT2 (PA 0x0000000000000000 - 0x00000000001FFFFF -> VA 0x0000010000000000 - 0x00000100001FFFFF)
; |       |- PD2[1] -> PT3 (PA 0x0000000000200000 - 0x00000000003FFFFF -> VA 0x0000010000200000 - 0x00000100003FFFFF)
; |- PML4[3-511] -> Unused