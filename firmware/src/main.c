#define F_CPU 1000000UL

// avoid redefinitoin of Attiny84a flag
// but also make intellisens avaiable for the attiny84a macros
#ifndef __AVR_ATtiny84A__ 
#define __AVR_ATtiny84A__ 
#endif 

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

typedef uint8_t u8;

u8 cnt = 0; 
u8 cnt_2 = 64; 
u8 cnt_3 = 128; 
u8 cnt_4 = 192; 

void setup_timer0(void)
{
    TCCR0A |= (1 << WGM00)  | (1 << WGM01) | // set fast-pwm mode
              (1 << COM0A1) | (1 << COM0B1); // non-inverting mode (clear oc0a on cmp match)

    TCCR0B |= (1 << CS00) | (1 << CS01);     // clk prescaler to N = 64 -> f_pwm = f_clk / N / 256 = 61,03515625

    TIMSK0 |= (1 << OCIE0A) | (1 << OCIE0B); // enable cmp match interrupt
}

void setup_timer1(void)
{
    TCCR1A |= (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1); // non-inverting mode
    TCCR1B |= (1 << WGM12) | (1 << CS10) | (1 << CS11);     // clk prescaler to N = 64

    TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);                // enable cmp match interrupt
}

int main(void)
{
    DDRB |= (1 << PB2);
    DDRA |= (1 << PA5) | (1 << PA6) | (1 << PA7);

    setup_timer0();
    setup_timer1();

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

ISR(TIM0_COMPB_vect)
{
    OCR0B = cnt_2++;
}

ISR(TIM1_COMPA_vect)
{
    OCR1A = cnt_3++;
}

ISR(TIM1_COMPB_vect)
{
    OCR1B = cnt_4++;
}
