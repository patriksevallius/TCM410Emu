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
	int8_t *flash = NULL;
	int8_t *ram = NULL;

	initialize_emulator(&ram, &flash);
	initialize_cpu(&cpu, ram, flash, FLASH_START);
    register_callbacks();

    for(;;)
    {
	    execute(&cpu);
    }
	return 0;
}
