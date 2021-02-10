#include <string.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "mcuio.h"
#include "miniprintf.h"

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>


static TaskHandle_t h_task2=0, h_task3=0;
static SemaphoreHandle_t h_mutex;

static volatile unsigned 
	rtc_isr_count = 0u,		    // Times rtc_isr() called
	rtc_alarm_count = 0u,		
	rtc_overflow_count = 0u;	// Times overflow occurred

static volatile unsigned 
	days=0,
	hours=0, minutes=0, seconds=0,
	alarm=0;			

static void mutex_lock(void) {
	xSemaphoreTake(h_mutex,portMAX_DELAY);
}

static void mutex_unlock(void) {
	xSemaphoreGive(h_mutex);
}

void rtc_isr(void) {
	UBaseType_t intstatus;
	BaseType_t woken = pdFALSE;

	++rtc_isr_count;
	if (rtc_check_flag(RTC_OW)) {
		++rtc_overflow_count;    // Timer overflowed:
		rtc_clear_flag(RTC_OW);
		if (!alarm)    		 	 // If no alarm pending, clear ALRF
			rtc_clear_flag(RTC_ALR);
	} 

	if (rtc_check_flag(RTC_SEC)) {
		// RTC tick interrupt:
		rtc_clear_flag(RTC_SEC);

		// Increment time:
		intstatus = taskENTER_CRITICAL_FROM_ISR();
		if (++seconds >= 60) {
			++minutes;
			seconds -= 60;
		}
		if (minutes >= 60) {
			++hours;
			minutes -= 60;
		}
		if (hours >= 24) {
			++days;
			hours -= 24;
		}
		taskEXIT_CRITICAL_FROM_ISR(intstatus);

		vTaskNotifyGiveFromISR(h_task2, &woken);
		portYIELD_FROM_ISR(woken);
		return;
	}

	if (rtc_check_flag(RTC_ALR)) {
		++rtc_alarm_count;
		rtc_clear_flag(RTC_ALR);

		vTaskNotifyGiveFromISR(h_task3,&woken);
		portYIELD_FROM_ISR(woken);
		return;
	}
}


static void task3(void *args __attribute__((unused))) {
	for (;;) {
		// Todo: gpioc change according to start/stop
		ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
	}
}


static void task2(void *args __attribute__((unused))) {

	for (;;) {
		ulTaskNotifyTake(pdTRUE,portMAX_DELAY); // block execution until notified

		gpio_toggle(GPIOC,GPIO13);
		mutex_lock();
		std_printf("Time: %3u days %02u:%02u:%02u\n",days,hours,minutes,seconds);
		mutex_unlock();
	}
}


static void set_time() {
	mutex_lock();
	std_printf("[+] Time has updating according to time on PC!\n");
	mutex_unlock();

	// Following code will not working: region `ram' overflowed by 380 bytes
	// time_t t;
	// struct tm time;
	// t = NULL;
	// time = *localtime(&t); 

    // Another way :) 
	long int unix_time = (unsigned long)time(NULL);
	long int extraTime;

	extraTime = unix_time % (24 * 60 * 60);

	hours   =  2+(extraTime / 3600);
	minutes =  (extraTime % 3600) / 60;
	seconds =  (extraTime % 3600) % 60;
}


static void rtc_setup(void) {
	/*
	 Initialization of RTC clock.
	*/	rcc_enable_rtc_clock();
		rtc_interrupt_disable(RTC_SEC);
		rtc_interrupt_disable(RTC_ALR);
		rtc_interrupt_disable(RTC_OW);


	/*
	 The HSE clock: 8 MHz crystal oscillator. 
	 The RTCCLK rate provided 8MHz/128=62.5 kHz. 
	*/	rtc_awake_from_off(RCC_HSE);
		rtc_set_prescale_val(62500);


    /*
	 RTC counter would be started at zero: 0xFFFFFFF0.
	 The RTC counter is 32 bits in size, so it overflows after counting to 0xFFFFFFFF. 
	*/	rtc_set_counter_val(0xFFFFFFF0);

	/*
	 Enables the interrupt controller for the rtc_isr()
	*/  nvic_enable_irq(NVIC_RTC_IRQ); 

		cm_disable_interrupts();
		rtc_clear_flag(RTC_SEC); 		// seconds tick
		rtc_clear_flag(RTC_ALR); 		// alarm
		rtc_clear_flag(RTC_OW);  		// overflow (32 bits in size)
		rtc_interrupt_enable(RTC_SEC);	
		rtc_interrupt_enable(RTC_ALR);	
		rtc_interrupt_enable(RTC_OW);	
		cm_enable_interrupts(); 		//  Ready to generate!

		set_time();
}


static void wait_terminal(void) {
	TickType_t ticks0, ticks;

	ticks0 = xTaskGetTickCount();

	for (;;) {
		ticks = xTaskGetTickCount();
		if (ticks - ticks0 > 1000) { 
			std_printf("Press any key to start...\n");
			ticks0 = ticks;
		}
		if (std_peek() >= 1) {
			while (std_peek() >= 1)
				std_getc(); 
			return;
		}
		taskYIELD();		
	}
}


/*********************************************************************
 * Task 1 : The user console task
 *********************************************************************/
static void task1(void *args __attribute__((unused))) {
	char ch;

	wait_terminal();
	std_printf("Terminal has started!\n\n");

	rtc_setup();
	taskYIELD();

	for (;;) {
		mutex_lock();
		std_printf("\nPress 'S' to synchronize PC time with stm32 RTC,\n"
			"or 'R' to read RTC current time.\n\n");
		
		mutex_unlock();
		
		ch = std_getc();

		if ( ch == 's' || ch == 'S' ) {
			mutex_lock();
			std_printf("\nUpdating time...\n");
			mutex_unlock();
			set_time();
		}
	}
}

int main(void) {

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC,GPIO_MODE_OUTPUT_50_MHZ,GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);

	h_mutex = xSemaphoreCreateMutex();
	xTaskCreate(task1,"task1",350,NULL,1,NULL);
	xTaskCreate(task2,"task2",400,NULL,3,&h_task2);
	xTaskCreate(task3,"task3",400,NULL,3,&h_task3);

	gpio_clear(GPIOC,GPIO13);

	usb_start(1,1);
	std_set_device(mcu_usb);

	vTaskStartScheduler();
	for (;;);

	return 0;
}