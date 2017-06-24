#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "emulator.h"

int32_t main(void)
{
	initialize_emulator(&cpu, "fw.bin");
	initialize_cpu(&cpu, FLASH_START);
    register_callbacks();

    for(;;)
    {
	    execute(&cpu);
    }
	return 0;
}
