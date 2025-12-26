#!/bin/bash
# Script to run the OS simulator with automatic process creation
cd /workspace/FixedPartitionMemory
echo -e "1\n8\n2" | timeout 30s ./os_simulator