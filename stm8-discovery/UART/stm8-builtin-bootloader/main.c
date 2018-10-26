#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm8s.h"
#include "stm8s_uart2.h"
#include "stm8s_wwdg.h"

void gpio_init (void)
{
    /* GPIOD reset */
  GPIO_DeInit(GPIOD);

  /* Configure PD0 (LED1) as output push-pull low (led switched on) */
  GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_FAST);
}

void uart2_init (void)
{
  UART2_DeInit();
  UART2_Init((uint32_t) 9600,
       UART2_WORDLENGTH_8D,
       UART2_STOPBITS_1,
       UART2_PARITY_NO,
       UART2_SYNCMODE_CLOCK_DISABLE,
       UART2_MODE_TXRX_ENABLE);
}

int putchar(int c)
{
  //Write a character to the UART2
  UART2_SendData8(c);

  //Loop until the end of transmission
  while (UART2_GetFlagStatus(UART2_FLAG_TXE) == RESET) ;

  return(c);
}

int getchar(void)
{
  uint8_t c = 0;

  /* Loop until the Read data register flag is SET */
  while (UART2_GetFlagStatus(UART2_FLAG_RXNE) == RESET) ;

  c = UART2_ReceiveData8();

  return (c);
}

void main (void) 
{
    char ans;

    int reset_counter = 0;

    gpio_init();
    uart2_init();

    // printf("BSL demo\n");

    while(1)
    {
        GPIO_WriteLow(GPIOD, GPIO_PIN_0);

        ans = getchar();

        if (ans == '#' && reset_counter == 0)
            reset_counter++;
        else if (ans == '#' && reset_counter == 1)
            reset_counter++;
        else if (ans == 'r' && reset_counter == 2)
            reset_counter++;
        else if (ans == 'e' && reset_counter == 3)
            reset_counter++;
        else if (ans == 's' && reset_counter == 4)
            reset_counter++;
        else if (ans == 'e' && reset_counter == 5)
            reset_counter++;
        else if (ans == 't' && reset_counter == 6)
            reset_counter++;
        else if (ans == '#' && reset_counter == 7)
            reset_counter++;
        else if (ans == '#' && reset_counter == 8)
            reset_counter++;
        else
            reset_counter = 0;

        // printf("%i", reset_counter); 

        if (reset_counter == 9)
        {
            // printf("\nReset to BSL!\n");
            WWDG_SWReset(); // reboot to bootloader
        }
    }
}
