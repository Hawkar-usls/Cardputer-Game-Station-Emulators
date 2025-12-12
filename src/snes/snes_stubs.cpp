#include "snes_stubs.h"
#include <Arduino.h>
#include <cstdio>

extern "C" {
    #include "snes9x/snes9x.h"
}

void S9xDeinitDisplay(void)
{
    // Nothing
}

void S9xExtraUsage()
{
    // Stub
}

void S9xExit()
{
    printf("[SNES] S9xExit called\n");
}

void S9xMessage(int type, int number, const char* message)
{
    (void)type;
    (void)number;
    printf("[SNES] %s\n", message ? message : "(null)");
}

bool S9xReadMousePosition(int32_t which1, int32_t *x, int32_t *y, uint32_t *buttons)
{
    (void)which1;
    (void)x;
    (void)y;
    (void)buttons;
    return false;
}

bool S9xReadSuperScopePosition(int32_t *x, int32_t *y, uint32_t *buttons)
{
    (void)x;
    (void)y;
    (void)buttons;
    return false;
}

bool JustifierOffscreen(void)
{
    return true;
}

void JustifierButtons(uint32_t *justifiers)
{
    (void)justifiers;
}

#ifdef SNES_NO_SOUND

void S9xAPUWritePort(uint8_t port, uint8_t value)
{
    (void)port;
    (void)value;
}

uint8_t S9xAPUReadPort(uint8_t port)
{
    (void)port;
    CPU.BranchSkip = true;
    return 0x00;
}

void S9xResetAPU(void)
{
    // TODO
}

#endif
