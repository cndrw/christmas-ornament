#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

int main()
{
    DDRA |= (1 << DDA7);

    while (1)
    {
        PORTA |= (1 << PORTA7);

        _delay_ms(500);

        PORTA &= ~(1 << PORTA7);

        _delay_ms(500);

    }
    return 0;
}