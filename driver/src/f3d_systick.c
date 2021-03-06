#include <f3d_systick.h>
#include <f3d_led.h> 
#include <f3d_user_btn.h>
#include <f3d_uart.h>

volatile int systick_flag = 0;
volatile int btn_flag = 0;
static int count = 0;

void f3d_systick_init(void) {
	SysTick_Config(SystemCoreClock/100);
}

void f3d_systick_toggle(int f){
	SysTick_Config(SystemCoreClock/f);
}

void SysTick_Handler(void) {	
  
}

/* f3d_systick.c ends here */
