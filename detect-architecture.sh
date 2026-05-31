#!/bin/bash

# Ensure gcc is installed
if ! command -v gcc &> /dev/null; then
    echo "Error: gcc is not installed." >&2
    exit 1
fi

FLAG=""

if gcc --version 2>&1 | grep -i -q "clang"; then
    # Clang (macOS/BSD) natively resolves to -target-cpu in its internal invocation
    MARCH_VALUE=$(gcc -### -E - -march=native 2>&1 | grep "cc1" | grep -oE '"-target-cpu" "[^"]+"' | head -n 1 | awk '{print $2}' | tr -d '"')
    FLAG="-target-cpu "
else
    # Genuine GCC. Extract the resolved -march flag from the raw 'cc1' execution line
    MARCH_VALUE=$(gcc -### -E - -march=native 2>&1 | grep "cc1" | grep -oE '"-march=[^"]+"' | head -n 1 | cut -d= -f2 | tr -d '"')
    FLAG="-march="
    
    # If empty or still "native", it's an ARM architecture using -mcpu
    if [ -z "$MARCH_VALUE" ] || [ "$MARCH_VALUE" = "native" ]; then
        MARCH_VALUE=$(gcc -### -E - -mcpu=native 2>&1 | grep "cc1" | grep -oE '"-mcpu=[^"]+"' | head -n 1 | cut -d= -f2 | tr -d '"')
		    FLAG="-mcpu="
    fi
fi

# Final safety check
if [ -z "$MARCH_VALUE" ] || [ "$MARCH_VALUE" = "native" ]; then
    echo "Error: Could not determine the architecture." >&2
    exit 1
fi

echo "${FLAG}${MARCH_VALUE}"
