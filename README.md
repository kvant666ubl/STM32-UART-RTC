# STM32-UART-RTC
Synchronizing the time of the MCU's built-in RTC with the computer and sending commands via serial communication.

## About
Project uses open-source embedded library [``libopencm3``](https://github.com/libopencm3/libopencm3), [``GNU-ARM``](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) Embedded Toolchain (v.10-2020-q4-major), [``st-flash``](https://github.com/stlink-org/stlink) utility to flash/read binary files to/from arbitrary sections of MCU memory, [``freertos``](https://www.freertos.org/) that provides multitasking, and [``openocd``](http://openocd.org/) debugger. 

Just click on them and follow the installation instructions, if you don't have it. This project assumes that you are using the libopencm3 library in the same directory.  

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


## Make and Flash

Using st-flash to erase previous firmware:
```
$ st-flash erase 
```
Out:
```
st-flash 1.6.1
2021-02-10T19:26:01 INFO common.c: F1xx Medium-density: 20 KiB SRAM, 128 KiB flash in at least 1 KiB pages.
Mass erasing
```

Remove files, if necessary:
```
$ make clobber
```
Out:
```
rm -f *.o *.d generated.* main.o rtos/heap_4.o rtos/list.o rtos/port.o rtos/queue.o rtos/tasks.o rtos/opencm3.o main.d rtos/heap_4.d rtos/list.d rtos/port.d rtos/queue.d rtos/tasks.d rtos/opencm3.d
rm -f *.elf *.bin *.hex *.srec *.list *.map 
```
And make install & upload to targer device:
```
$ make all && make flash
```
Out:

```
.   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
.   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
2021-02-10T19:28:21 INFO common.c: Flash page at addr: 0x08002800 erased
2021-02-10T19:28:21 INFO common.c: Flash page at addr: 0x08002c00 erased
2021-02-10T19:28:21 INFO common.c: Flash page at addr: 0x08003000 erased
2021-02-10T19:28:21 INFO common.c: Flash page at addr: 0x08003400 erased
2021-02-10T19:28:21 INFO common.c: Flash page at addr: 0x08003800 erased
2021-02-10T19:28:21 INFO common.c: Finished erasing 15 pages of 1024 (0x400) bytes
2021-02-10T19:28:21 INFO common.c: Starting Flash write for VL/F0/F3/F1_XL core id
2021-02-10T19:28:21 INFO flash_loader.c: Successfully loaded flash loader in sram 15/15 pages written
2021-02-10T19:28:22 INFO common.c: Starting verification of write complete
2021-02-10T19:28:22 INFO common.c: Flash written and verified! jolly good!
```
Connect to board:
```
$ tio /dev/ttyACM0
```
Output, press 's':
```
Updating time...
[+] Time has updating according to time on PC!

Press 'S' to synchronize PC time with stm32 RTC,
or 'R' to read RTC current time.

Time:   0 days 12:13:08
Time:   0 days 12:13:09
Time:   0 days 12:13:10
Time:   0 days 12:13:11

```
Jolly good! :)
