# STM32-UART-RTC
Synchronizing the time of the MCU's built-in RTC with the computer and sending commands via serial communication.

## About
Project uses open-source embedded library [``libopencm3``](https://github.com/libopencm3/libopencm3), [``GNU-ARM``](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) Embedded Toolchain (v.10-2020-q4-major), [``st-flash``](https://github.com/stlink-org/stlink) utility to flash/read binary files to/from arbitrary sections of MCU memory, [``freertos``](https://www.freertos.org/) that provides multitasking, and [``openocd``](http://openocd.org/) debugger. 

Just click on them and follow the installation instructions, if you don't have it.

## Current version
Current version of project using stm32f103c8t6 as target device. Linker script ``stm32f103c8t6.ld`` includes divisions of ram/flash section and configurates of vectors and stack. 

The ``main.c`` complilation unit includes ``rcc.h`` and ``rtc.h`` to make clock configuration to blink built-in LED and start ``rtc_isr_counter``. 

The clock source used by this project is the ``HSE`` clock of board, which is selected by the following libopencm3 function call:

```cpp
rtc_awake_from_off(RCC_HSE);
```

The counter initialized with the following call:
```cpp
rtc_set_counter_val(0xFFFFFFF0);
```

And at the main code also presents the user console task, toggle LED and report time task and additional task to creating another user get values.


The program can be run using a USB-based TTL UART or directly over a USB cable.
