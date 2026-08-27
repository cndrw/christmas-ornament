#define F_CPU 1000000UL
#define __AVR_ATtiny84A__

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

typedef uint8_t u8;

u8 cnt = 0; 

int main()
{
    DDRB |= (1 << PB2);
    TCCR0A |= (1 << WGM00) | (1 << WGM01) | // set fast-pwm mode
              (1 << COM0A1);                // non-inverting mode (clear oc0a on cmp match)

    TCCR0B |= (1 << CS00) | (1 << CS01);    // clk prescaler to 64 -> 10^6 / 64 = 15625 

    OCR0A = 0xFF;                            // 50% duty cycle

    TIMSK0 |= (1 << OCIE0A);                // enable cmp match interrupt

    sei();  // enable interrupt

    while (1)
    {
        _delay_ms(10);
    }

    return 0;
}

ISR(TIM0_COMPA_vect)
{
    OCR0A = cnt++;
}