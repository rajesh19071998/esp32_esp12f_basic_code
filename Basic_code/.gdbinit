# Load firmware symbols
file .pio/build/esp32dev/firmware.elf

# Connect to OpenOCD
target remote :3333

# Reset and halt the target
monitor reset halt

# Set initial breakpoint at Arduino setup()
break setup

# Optional: also break at loop()
break loop

# Show source around current line
list

# Continue execution until breakpoint
continue
