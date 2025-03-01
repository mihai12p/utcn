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
GIGABYTE                    equ (1024 * (MEGABYTE))
TERABYTE                    equ (1024 * (GIGABYTE))

TOP_OF_STACK                equ (0x200000)

FLAT_DESCRIPTOR_CODE64      equ 0x002F9A000000FFFF  ; Code: Execute/Read
FLAT_DESCRIPTOR_DATA64      equ 0x00CF92000000FFFF  ; Data: Read/Write
FLAT_DESCRIPTOR_CODE32      equ 0x00CF9A000000FFFF  ; Code: Execute/Read
FLAT_DESCRIPTOR_DATA32      equ 0x00CF92000000FFFF  ; Data: Read/Write
FLAT_DESCRIPTOR_CODE16      equ 0x00009B000000FFFF  ; Code: Execute/Read, accessed
FLAT_DESCRIPTOR_DATA16      equ 0x000093000000FFFF  ; Data: Read/Write, accessed

;
; https://coconucos.cs.uni-duesseldorf.de/lehre/abschlussarbeiten/BA-Urlacher-23.pdf
;
LAPIC_BASE                  equ (0x1000000)
AP_STARTUP_CODE_ADDRESS     equ (0x2000)
LAPIC_ID_REGISTER           equ (0x20)
LAPIC_VERSION_REGISTER      equ (0x30)
LAPIC_EOI_REGISTER          equ (0xB0)
LAPIC_SVR_OFFSET            equ (0xF0)             ; Spurious Interrupt Vector Register offset
LAPIC_ESR_OFFSET            equ (0x280)            ; Error Status Register offset
LAPIC_ICR_LOW_OFFSET        equ (0x300)            ; ICR low register offset
LAPIC_ICR_HIGH_OFFSET       equ (0x310)            ; ICR high register offset
APIC_ENABLED                equ (0x100)            ; APIC Enable bit (bit 8)
INIT_DELIVERY               equ (0x500)            ; INIT delivery mode
SIPI_DELIVERY               equ (0x600)            ; Startup IPI delivery mode

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
