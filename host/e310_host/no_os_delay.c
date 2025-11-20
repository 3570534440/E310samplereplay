#include "no_os_delay.h"
#include <unistd.h>

/* Generate microseconds delay. */
void no_os_udelay(uint32_t usecs)
{
    usleep(usecs);
}

/* Generate miliseconds delay. */
void no_os_mdelay(uint32_t msecs)
{
    usleep(msecs * 1000);
}
