# Notes:


## Checklist:

- [x] Input capturing on pin PA6
- [x] Systick measuring ms or higher resolution
- [x] ringbuffer and window matching
- [x] pattern matching
- [x] Pattern set and upload via uart
- [ ] Universal index sending
- [ ] Button states
  - [ ] Hold 5 sec -> erase
  - [ ] push -> save address mode ?
- [ ] Flash read and write on last page


## RTC

Add RTC support for periodic data logging or similar information.




# DOCS


# Frequencies:
This chip is designed to work in the 434 Mhz frequency range. A primary goal is to support the protocols specified in `docs/prots`.

# Modes of operation:

Interface mode: 
- The chip is primarily controlled via the `UART1` control wires
- The chip continuously scans the 434Mhz band and streams out any _indices_ of patterns that have been matched
- The chip can be used as a "sender"
- The section `SYSTEM_RUN_DATA` has **no function** in this mode. In general the purpose of this system is to be "as stateless as possible". Patterns may be stored in `SIGNAL_PATTERNS_DATA` section, but this is not needed (and only saves startup time)

Client/Standalone mode:
- The configuration bit inside `SYSTEM_RUN_DATA` specifies the kind of device and its functionality 
- `SYSTEM_RUN_DATA` stores the current state should it need to be restored on unexpected power loss
- It is **expected** that the signals stored inside `SIGNAL_PATTERNS_DATA` **DO NOT CHANGE** and remain at factory settings at all times. 
- `SYSTEM_RUN_DATA` also stores 'potential addresses' that the chip should currently respond to (backwards compatibility)
- UART is turned **off**, it should only send information during debugging


# Memory organization:
This is designed for the stm32f030x6 class of chips, they feature a cortex-m0 CPU.

Total memory: 16k flash + 4k RAM
Sections: Specified in `scripts/STM32F030F4PX_FLASH.ld`

Flash starts at address `0x8000000` and is organized into 1k pages.
- Pages can only be written to _once_ after they've been erased (otherwise it the cpu will crash)
- Only whole pages can be erased
- Erases have a limited cycle of about 100 000

SIGNAL_PATTERNS_DATA Starts at `0x8000000 + ( 16k - 2k)` -> Its the second page counted from the back
- This page retains the period patterns in client mode
- It _may_ retain period patterns in interface mode


SYSTEM_RUN_DATA Starts at `0x8000000 + ( 16k - 1k)` -> Last page inside the flash
- Retains current state of the system in client mode
  - Note: As **few writes as possible** should happen on this memory, the number of total erases are limited per chip
- No function in interface mode



