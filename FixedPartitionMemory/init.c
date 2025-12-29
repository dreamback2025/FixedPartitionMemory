#include "os_types.h"
#include "log.h"
#include "process.h"
#include "partition.h"
#include "memory.h"
#include "config.h"
#include "kernel.h"

// È«¾ÖÄÚ´æ×´Ì¬
static uint8_t system_memory[MEMORY_SIZE];
static uint32_t current_time = 0;

