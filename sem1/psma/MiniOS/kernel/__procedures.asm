
%include "Utils.inc"

EXPORT2C EnableLAPIC, SendStartupIPI

segment .text

[BITS 64]
EnableLAPIC:
    mov     eax,    LAPIC_BASE + LAPIC_SVR_OFFSET
    mov     ecx,    [eax]
    and     ecx,    0xFFFFFF00
    or      ecx,    0xFF
    or      ecx,    APIC_ENABLED
    mov     [eax],  ecx
    ret

;
; https://wiki.osdev.org/Symmetric_Multiprocessing#BSP_Initialization_Code
;
SendStartupIPI:
    push    rbx
    mov     ebx,    ecx

    mov     esi,    LAPIC_BASE + LAPIC_ESR_OFFSET
    mov     eax,    0
    mov     [esi],  eax

    ;
    ; Send INIT IPI
    ;
    mov     esi,    LAPIC_BASE + LAPIC_ICR_HIGH_OFFSET
    mov     eax,    ebx
    shl     eax,    24
    mov     [esi],  eax

    mov     esi,    LAPIC_BASE + LAPIC_ICR_LOW_OFFSET
    mov     eax,    0xC500
    mov     [esi],  eax

    call    __wait_for_delivery

    mov     esi,    LAPIC_BASE + LAPIC_ICR_HIGH_OFFSET
    mov     eax,    ebx
    shl     eax,    24
    mov     [esi],  eax

    mov     esi,    LAPIC_BASE + LAPIC_ICR_LOW_OFFSET
    mov     eax,    0x8500
    mov     [esi],  eax

    call    __wait_for_delivery
    call    __delay

    mov     esi,    LAPIC_BASE + LAPIC_ESR_OFFSET
    mov     eax,    0
    mov     [esi],  eax

    ;
    ; First SIPI
    ;
    mov     esi,    LAPIC_BASE + LAPIC_ICR_HIGH_OFFSET
    mov     eax,    ebx
    shl     eax,    24
    mov     [esi],  eax

    mov     esi,    LAPIC_BASE + LAPIC_ICR_LOW_OFFSET
    mov     eax,    SIPI_DELIVERY | (AP_STARTUP_CODE_ADDRESS / PAGE_SIZE)
    mov     [esi],  eax

    call    __short_delay
    call    __wait_for_delivery

    ;
    ; Second SIPI
    ;
    mov     esi,    LAPIC_BASE + LAPIC_ICR_HIGH_OFFSET
    mov     eax,    ebx
    shl     eax,    24
    mov     [esi],  eax

    mov     esi,    LAPIC_BASE + LAPIC_ICR_LOW_OFFSET
    mov     eax,    SIPI_DELIVERY | (AP_STARTUP_CODE_ADDRESS / PAGE_SIZE)
    mov     [esi],  eax

    call    __short_delay
    call    __wait_for_delivery

    pop     rbx
    ret

__wait_for_delivery:
    mov     esi,    LAPIC_BASE + LAPIC_ICR_LOW_OFFSET
.check:
    mov     eax,    [esi]
    test    eax,    0x1000                   ; test delivery status bit
    jnz     .check
    ret

__delay:                                     ; 10ms delay
    mov     rcx,    1000000
.loop:
    dec     ecx
    test    ecx,    ecx
    jnz     .loop
    ret

__short_delay:                               ; 200µs delay
    mov     rcx,    20000
.loop:
    dec     ecx
    test    ecx,    ecx
    jnz     .loop
    ret