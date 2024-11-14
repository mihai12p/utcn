#include "Edit.h"

static int gInEditMode = 0;

int
IsInEditMode()
{
    return gInEditMode == 1;
}

VOID
SetEditMode(
    _In_ int EditMode
)
{
    gInEditMode = EditMode;
}