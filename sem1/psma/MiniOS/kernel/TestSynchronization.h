#ifndef _TEST_SYNCHRONIZATION_H_
#define _TEST_SYNCHRONIZATION_H_

#include "main.h"

VOID
TestcaseSynchronizedPrint(
    _In_opt_ const char* Arguments
);

VOID
TestcaseLinkedList(
    _In_opt_ const char* Arguments
);

VOID
APTestcaseSynchronizedPrint();

VOID
APTestcaseLinkedList();

#endif//_TEST_SYNCHRONIZATION_H_