#include <stdio.h>
#include <stdint.h>
#include <string.h>

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
    printf("=== Memory Partition Analysis ===\n");
    printf("Total memory: %d bytes\n", MEMORY_SIZE);
    printf("OS partition: %d bytes\n", OS_PARTITION_SIZE);
    printf("Available memory: %d bytes\n\n", MEMORY_SIZE - OS_PARTITION_SIZE);
    
    printf("Fixed partition sizes: ");
    for (int i = 0; i < FIXED_PARTITION_COUNT; i++) {
        printf("%d ", FIXED_PARTITION_SIZES[i]);
    }
    printf("\n\n");
    
    // Simulate the NEW partition initialization algorithm
    printf("Simulating NEW partition_init() algorithm:\n");
    
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

    printf("Starting from address: %d (0x%x)\n", current_addr, current_addr);
    
    // For each size, create a predetermined number of partitions
    uint32_t allocated_partitions[FIXED_PARTITION_COUNT] = {0};
    
    for (uint32_t size_idx = 0; size_idx < FIXED_PARTITION_COUNT && partition_idx < MAX_PARTITIONS - 1; size_idx++) {
        uint32_t partition_size = FIXED_PARTITION_SIZES[size_idx];
        
        // Calculate how many partitions of this size we can create
        uint32_t base_count = 1;  // Each size at least 1
        if (partition_size == 64) base_count = 6;  // Small partitions: more of them
        else if (partition_size == 128) base_count = 3;  // Medium: fewer
        else if (partition_size == 256) base_count = 2;  // Large: even fewer
        else if (partition_size == 512) base_count = 1;  // Largest: just 1
        
        printf("\nFor partition size %d:\n", partition_size);
        printf("  Want to create: %d partitions\n", base_count);
        
        // Check if we have enough memory
        uint32_t remaining_memory_after_os = MEMORY_SIZE - current_addr;
        uint32_t max_possible = remaining_memory_after_os / partition_size;
        printf("  Max possible with remaining memory: %d\n", max_possible);
        
        if (base_count > max_possible) {
            base_count = max_possible;
            printf("  Limited to: %d partitions\n", base_count);
        }
        
        // Check if we have space in partition table
        uint32_t max_in_table = MAX_PARTITIONS - partition_idx;
        if (base_count > max_in_table) {
            base_count = max_in_table;
            printf("  Limited by partition table to: %d partitions\n", base_count);
        }
        
        // Create the partitions
        for (uint32_t i = 0; i < base_count && partition_idx < MAX_PARTITIONS - 1 && 
             current_addr + partition_size <= MEMORY_SIZE; i++) {
            partition_table[partition_idx].start = current_addr;
            partition_table[partition_idx].size = partition_size;
            partition_table[partition_idx].state = PARTITION_FREE;
            partition_table[partition_idx].owner_pid = 0;
            partition_table[partition_idx].next = &partition_table[partition_idx + 1];
            
            printf("  Created partition %d: start=0x%x (addr %d), size=%d\n", 
                   partition_idx, current_addr, current_addr, partition_size);
            
            current_addr += partition_size;
            partition_idx++;
            allocated_partitions[size_idx]++;
        }
    }
    
    // Use remaining space for small partitions
    printf("\nUsing remaining space for small partitions:\n");
    while (partition_idx < MAX_PARTITIONS - 1 && current_addr < MEMORY_SIZE) {
        uint32_t smallest_size = FIXED_PARTITION_SIZES[0];
        if (current_addr + smallest_size <= MEMORY_SIZE) {
            partition_table[partition_idx].start = current_addr;
            partition_table[partition_idx].size = smallest_size;
            partition_table[partition_idx].state = PARTITION_FREE;
            partition_table[partition_idx].owner_pid = 0;
            partition_table[partition_idx].next = &partition_table[partition_idx + 1];
            
            printf("  Created small partition %d: start=0x%x (addr %d), size=%d\n", 
                   partition_idx, current_addr, current_addr, smallest_size);
            
            current_addr += smallest_size;
            partition_idx++;
        } else {
            // Create one final partition with remaining space
            uint32_t remaining_size = MEMORY_SIZE - current_addr;
            if (remaining_size > 0) {
                partition_table[partition_idx].start = current_addr;
                partition_table[partition_idx].size = remaining_size;
                partition_table[partition_idx].state = PARTITION_FREE;
                partition_table[partition_idx].owner_pid = 0;
                partition_table[partition_idx].next = NULL;
                
                printf("  Created final partition %d: start=0x%x (addr %d), size=%d\n", 
                       partition_idx, current_addr, current_addr, remaining_size);
                
                partition_idx++;
            }
            break;
        }
    }
    
    if (partition_idx > 0) {
        partition_table[partition_idx - 1].next = NULL;
    }
    
    printf("\nTotal partitions created: %d\n", partition_idx);
    printf("\nPartition summary:\n");
    printf("Index  Start    End      Size  State  PID\n");
    printf("-----  -----    ---      ----  -----  ---\n");
    for (int i = 0; i < partition_idx; i++) {
        const char* state_str = (partition_table[i].state == PARTITION_OS) ? "OS" : 
                               (partition_table[i].state == PARTITION_ALLOCATED) ? "ALLOC" : "FREE";
        printf("%5d  0x%04x   0x%04x   %4d  %s    %d\n", 
               i, 
               partition_table[i].start, 
               partition_table[i].start + partition_table[i].size - 1, 
               partition_table[i].size, 
               state_str,
               partition_table[i].owner_pid);
    }
    
    printf("\n=== Process Memory Allocation Test ===\n");
    // Test if processes with different memory requirements can be allocated
    int test_sizes[] = {50, 80, 124, 142, 152, 200, 300};
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    printf("Testing allocation for processes requiring:\n");
    for (int i = 0; i < num_tests; i++) {
        printf("  Process %d: %d bytes -> ", i+1, test_sizes[i]);
        
        // Find first free partition that can fit this size
        int found = 0;
        for (int j = 0; j < partition_idx; j++) {
            if (partition_table[j].state == PARTITION_FREE && 
                partition_table[j].size >= test_sizes[i]) {
                printf("Allocated to partition %d (size %d)\n", j, partition_table[j].size);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("FAILED - No suitable partition found\n");
        }
    }
    
    return 0;
}