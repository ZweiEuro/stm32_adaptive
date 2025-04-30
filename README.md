# Notes:


## Checklist:

Current problems:
- The receiver circuit is flaky as shit and produces all kinds of problematic input that crashes the chip or just causes indefinite program stalling and starves main thread
- Button needs to be connected
- WS28 interface connector
- dimmer connector
- relay connector




## RTC

Add RTC support for periodic data logging or similar information.




# DOCS

# Hardware:

Timer units, grouped by type: 
- TIM1: Advanced, inaccessible on demo board due to pin placement and uart connections
- TIM2/3: General purpose; 
- TIM14: General purpose; 
- TIM15/16/17: General purpose; **tim15 does not exist on stm32F030**; 
- TIM6/7: General purpose; **do not exist on stm32f030**

All TIM units: TIM1, TIM3, TIM14, TIM16, TIM17

PA6 -> Input capture with `TIM3`
PA0 -> 434 output with `TIM14`
PA7 -> WS2815 dataline `TIM17_CH1`




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



