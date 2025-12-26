#include <stdio.h>
#include <stdint.h>

// Define the same constants as in the actual code
#define MEMORY_SIZE 1024
#define OS_PARTITION_SIZE 128
#define MAX_PARTITIONS 16
#define MAX_PROCESSES 32

typedef enum {
    PARTITION_FREE = 0,
    PARTITION_ALLOCATED = 1,
    PARTITION_OS = 2
} partition_state_t;

typedef struct {
    uint32_t start;
    uint32_t size;
    partition_state_t state;
    uint32_t owner_pid;
    struct partition* next;
} partition_t;

// 预定义固定分区大小
static const uint32_t FIXED_PARTITION_SIZES[] = {64, 128, 256, 512};
#define FIXED_PARTITION_COUNT (sizeof(FIXED_PARTITION_SIZES) / sizeof(FIXED_PARTITION_SIZES[0]))

partition_t partition_table[MAX_PARTITIONS];

int main() {
    printf("Memory analysis:\n");
    printf("Total memory: %d bytes\n", MEMORY_SIZE);
    printf("OS partition: %d bytes\n", OS_PARTITION_SIZE);
    printf("Available memory: %d bytes\n\n", MEMORY_SIZE - OS_PARTITION_SIZE);
    
    printf("Fixed partition sizes: ");
    for (int i = 0; i < FIXED_PARTITION_COUNT; i++) {
        printf("%d ", FIXED_PARTITION_SIZES[i]);
    }
    printf("\n\n");
    
    // Simulate the partition initialization algorithm
    printf("Simulating partition_init() algorithm:\n");
    
    // Reset partition table
    for (uint32_t i = 0; i < MAX_PARTITIONS; i++) {
        partition_table[i].state = PARTITION_FREE;
        partition_table[i].next = NULL;
    }

    // Create OS partition
    partition_table[0].start = 0;
    partition_table[0].size = OS_PARTITION_SIZE;
    partition_table[0].state = PARTITION_OS;
    partition_table[0].owner_pid = 0;

    // Create fixed partitions - from remaining memory
    uint32_t current_addr = OS_PARTITION_SIZE;
    uint32_t partition_idx = 1;

    printf("Starting from address: %d\n", current_addr);
    
    // For each size, create partitions
    for (uint32_t size_idx = 0; size_idx < FIXED_PARTITION_COUNT && partition_idx < MAX_PARTITIONS - 1; size_idx++) {
        uint32_t partition_size = FIXED_PARTITION_SIZES[size_idx];
        
        printf("\nTrying to create partitions of size %d:\n", partition_size);
        
        // Calculate number of partitions of this size that fit in remaining memory
        uint32_t remaining_memory = MEMORY_SIZE - current_addr;
        uint32_t num_partitions = remaining_memory / partition_size;
        
        printf("  Remaining memory: %d, partition size: %d, num partitions possible: %d\n", 
               remaining_memory, partition_size, num_partitions);
        
        // Limit to avoid exceeding partition table size
        if (num_partitions > (MAX_PARTITIONS - partition_idx - 1)) {
            num_partitions = MAX_PARTITIONS - partition_idx - 1;
            printf("  Limited to %d partitions to avoid exceeding table size\n", num_partitions);
        }
        
        // Create fixed size partitions
        for (uint32_t i = 0; i < num_partitions && partition_idx < MAX_PARTITIONS - 1; i++) {
            partition_table[partition_idx].start = current_addr;
            partition_table[partition_idx].size = partition_size;
            partition_table[partition_idx].state = PARTITION_FREE;
            partition_table[partition_idx].owner_pid = 0;
            partition_table[partition_idx].next = &partition_table[partition_idx + 1];
            
            printf("  Created partition %d: start=0x%x, size=%d\n", 
                   partition_idx, current_addr, partition_size);
            
            current_addr += partition_size;
            partition_idx++;
        }
    }
    
    printf("\nTotal partitions created: %d\n", partition_idx);
    printf("\nPartition summary:\n");
    for (int i = 0; i < partition_idx; i++) {
        const char* state_str = (partition_table[i].state == PARTITION_OS) ? "OS" : 
                               (partition_table[i].state == PARTITION_ALLOCATED) ? "ALLOC" : "FREE";
        printf("  %d: start=0x%03x, size=%d, state=%s\n", 
               i, partition_table[i].start, partition_table[i].size, state_str);
    }
    
    return 0;
}