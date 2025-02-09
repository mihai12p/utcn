
%include "Utils.inc"

IMPORTFROMC PML4, APMain
EXPORT2C APStart, GetLAPICId

segment .text

[BITS 16]
APStart:
    xor  ax,  ax       ; start setting up a context
    mov  ds,  ax
    mov  es,  ax
    mov  gs,  ax
    mov  ss,  ax
    mov  fs,  ax

    ;
    ; starting RM to PM32 transition
    ;
    cli

    ;
    ; load GDT (the one from ssl.asm)
    ;
    lgdt [0x7ED4]

    ;
    ; enable protected mode by setting CR0.PE
    ; DON'T use macro since the stack is not set yet
    ;
    mov  eax,       CR0
    or   eax,       CR0.PE
    mov  CR0,       eax

    ;
    ; change the CS to 24 (index of FLAT_DESCRIPTOR_CODE32 entry)
    ;
    jmp  24:AP_STARTUP_CODE_ADDRESS + (.bits32 - APStart)

[BITS 32]
.bits32:
    mov  ax,    32       ; index of FLAT_DESCRIPTOR_DATA32 entry
    mov  ds,    ax
    mov  es,    ax
    mov  gs,    ax
    mov  ss,    ax
    mov  fs,    ax

    cli

    mov  esp,   TOP_OF_STACK - PAGE_SIZE        ; just below the first CPU's stack

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

    ;
    ; setup some stack (1 PAGE) for each CPU
    ;
    call GetLAPICId
    shl  eax,   12
    mov  esp,   TOP_OF_STACK - PAGE_SIZE        ; just below the first CPU's stack
    sub  esp,   eax

    ;
    ; change the CS to 8 (index of FLAT_DESCRIPTOR_CODE64 entry)
    ;
    jmp  8:AP_STARTUP_CODE_ADDRESS + (.bits64 - APStart)

[BITS 64]
.bits64:
    mov  ax,    16       ; index of FLAT_DESCRIPTOR_DATA64 entry
    mov  ds,    ax
    mov  es,    ax
    mov  gs,    ax
    mov  ss,    ax
    mov  fs,    ax

    call __invalidate_TLB

    mov  rax,   APMain
    call rax

    break
    cli
    hlt

__invalidate_TLB:
    ;
    ; invalidate all entries in the TLB (Translation Lookaside Buffer)
    ; where the processor may cache information about the translation of linear addresses
    ;
    mov  rax,    cr3
    mov  cr3,    rax
    ret

[BITS 32]
GetLAPICId:
    ;
    ; read LAPIC ID Register
    ;
    mov eax, LAPIC_BASE + LAPIC_ID_REGISTER
    mov eax, [eax]

    ;
    ; extract LAPIC ID (bits 24-31)
    ;
    shr eax, 24
    ret