;
; DEFINES
;
%define break               xchg bx, bx
%define BIT(x)              (1 << (x))

;
; CONSTANTS
;
PAGE_SIZE                   equ (1 << 12)
PAGE_MASK                   equ ~(PAGE_SIZE - 1)

BIT_PRESENT                 equ BIT(0)
BIT_READ_WRITE              equ BIT(1)
BIT_USER_SUPERVISOR         equ BIT(2)

CR0.PG                      equ BIT(31)
CR0.PE                      equ BIT(0)
CR4.PAE                     equ BIT(5)
CR4.OSFXSR                  equ BIT(9)

IA32_EFER_MSR               equ (0xC0000080)
IA32_EFER_MSR.LME           equ BIT(8)

KILOBYTE                    equ (1024)
MEGABYTE                    equ (1024 * (KILOBYTE))
GIGABYTE                    equ (1024 * (GIGABYTE))
TERABYTE                    equ (1024 * (TERABYTE))

TOP_OF_STACK                equ (0x200000)

;
; MACROS
;
%macro IMPORTFROMC 1-*
    %rep %0
        %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
            extern _%1
            %1 equ _%1
        %else
            extern %1
        %endif
        %rotate 1 
    %endrep
%endmacro

%macro EXPORT2C 1-*
    %rep %0
        %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
            global _%1
            _%1 equ %1
        %else
            global %1
        %endif
        %rotate 1 
    %endrep
%endmacro

%macro DEFINE_PT 2
    %define PT_NAME %1
    %assign START_ADDR %2

    align PAGE_SIZE
    PT_NAME:
        %rep 512
            dq START_ADDR | BIT_PRESENT | BIT_READ_WRITE    ; PAGE_ENTRY
            %assign START_ADDR START_ADDR + PAGE_SIZE
        %endrep
%endmacro

%macro ENABLE_CR_FLAG 2
    %define CTRL_REG %1
    %assign FLAG %2

    push eax
    mov  eax,       CTRL_REG
    or   eax,       FLAG
    mov  CTRL_REG,  eax
    pop  eax
%endmacro

%macro ISR_NOERRCODE 1
    isr_stub_%+%1:
        push 0
        push %1
        jmp  __isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
    isr_stub_%+%1:
        push %1
        jmp  __isr_common_stub
%endmacro
