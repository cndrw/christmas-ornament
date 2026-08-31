#define F_CPU 1000000UL

// avoid redefinition of Attiny84a flag
// but also make intellisens avaiable for the attiny84a macros
#ifndef __AVR_ATtiny84A__ 
#define __AVR_ATtiny84A__ 
#endif 

#include <avr/io.h>
#include <avr/interrupt.h>

#define __DELAY_BACKWARD_COMPATIBLE__ // make it possible to use _delay_ms with variables
#include <util/delay.h>

#define MS_TO_TICK_CONVERTION_FACTOR 0.06103515625
#define TICK_TO_MS_CONVERTION_FACTOR (1 / MS_TO_TICK_CONVERTION_FACTOR)
#define S_TO_TICK(x)  (x *  61.03515625)
#define MS_TO_TICK(x) (x *  MS_TO_TICK_CONVERTION_FACTOR)

#define N_LED ((sizeof(leds) / sizeof(leds[0])))
#define SCHEDULE_SIZE ((sizeof(schedule) / sizeof(schedule[0])))

#define MAX(x, y) (x > y ? x : y)

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;

typedef struct {
    u8 pin;
    volatile u8 value;
}led_t;

typedef struct {
    led_t* led;
    u8 enabled;
    u8 t;
    i8 direction;
    u16 delay_ms;
}sparkle_blink_t;

volatile u64 ticks = 0; 

led_t leds[] = {
    { .pin = PB2, .value = 0 },
    { .pin = PA5, .value = 0 },
    { .pin = PA6, .value = 0 },
    { .pin = PA7, .value = 0 }
};

sparkle_blink_t sparke_blinks[] = {
    { .led = &leds[0], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 500 },
    { .led = &leds[1], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 900 },
    { .led = &leds[2], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 400 },
    { .led = &leds[3], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 700 }
};

u8 schedule[] = { 1, 2, 3 };

u32 seedx32 = 341259264;
u32 xorshift32()
{
    seedx32 ^= seedx32 << 13;
    seedx32 ^= seedx32 >> 17;
    seedx32 ^= seedx32 << 5;
    return seedx32 + TCCR1A * 2 - TCCR0B + TCCR0A;
}

u64 get_time_ms(void)
{
    return ticks * TICK_TO_MS_CONVERTION_FACTOR;
}

void shuffle(u8* const array, u32 n)
{
    if (n > 1) 
    {
        for (u32 i = 0; i < n - 1; i++) 
        {
        //   u32 j = i + rand() / (RAND_MAX / (n - i) + 1);
        //   u8 j = i + TCCR0A / (255 / (n - i) + 1);
          u8 j = i + xorshift32() / (UINT32_MAX / (n - i) + 1);
          u8 t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void setup_timer0(void)
{
    TCCR0A |= (1 << WGM00)  | (1 << WGM01) | // set fast-pwm mode
              (1 << COM0A1) | (1 << COM0B1); // non-inverting mode (clear oc0a on cmp match)

    TCCR0B |= (1 << CS00) | (1 << CS01);     // clk prescaler to N = 64 -> f_pwm = f_clk / N / 256 = 61,03515625

    TIMSK0 |= (1 << TOIE0); // overflow interrupt
}

void setup_timer1(void)
{
    TCCR1A |= (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1); // non-inverting mode
    TCCR1B |= (1 << WGM12) | (1 << CS10) | (1 << CS11);     // clk prescaler to N = 64

    TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);                // enable cmp match interrupt
}

u8 ease_in_cubic(const u8 x)
{
    return (u32)x * x * x / 65025;
}

u8 led_blink(sparkle_blink_t* const handle)
{
    handle->led->value = ease_in_cubic(handle->t);

    if (handle->t == UINT8_MAX) handle->direction = -1;
    if (handle->t == 0) handle->direction = 1;

    handle->t += handle->direction;

    return handle->t == 0;
}

int main(void)
{
    DDRB |= (1 << PB2);
    DDRA |= (1 << PA5) | (1 << PA6) | (1 << PA7);

    setup_timer0();
    setup_timer1();

    sei();  // enable interrupt

    u64 last_update = get_time_ms();    
    u64 last_tick = ticks;

    u8 idx = 0;
    u8 unused_led = 0;

    sparke_blinks[0].enabled = 1;

    // Anforderungen
    // 1. nach dem starten einer led gibt es eine minimal zeit in der keine andere led gestartet werden darf (nie snychrone leds) 
    // 2. es sollte vermieden werden dass manche leds zu lange nicht geblinkt haben (gute distribution)
    while (1)
    {
        // all leds OFF except 1
        // leds[0].duty_cycle = 128;

        // schedule sagt wie die reihenfolge der leds ist, und jede led hat ein delay ab wann sie dann angeht 
        if (!sparke_blinks[schedule[idx]].enabled && get_time_ms() - last_update > sparke_blinks[schedule[idx]].delay_ms)
        {
            sparke_blinks[schedule[idx]].enabled = 1;
            idx++;
            // wenn die letzte led anfängt muss der schedule neu geshuffelt werden (ohne die momentane led)
            if (idx == 3)
            {
                // reshuffle schedule
                u8 tmp = 0;
                tmp = schedule[2];
                schedule[2] = unused_led;
                unused_led = tmp;

                shuffle(schedule, SCHEDULE_SIZE);
                idx = 0;
                for (u8 i = 0; i < N_LED; i++)
                {
                    // sparke_blinks[i].delay_ms = 200;
                    sparke_blinks[schedule[idx]].delay_ms = (xorshift32() / UINT32_MAX) * 500 + 40;
                }
            }

            last_update = get_time_ms();
        }

        // 100ms -> led 1 an

        for (u8 i = 0; i < N_LED; i++)
        {
            if(sparke_blinks[i].enabled && led_blink(&sparke_blinks[i]))
            {
                sparke_blinks[i].enabled = 0;
            }
        }

        _delay_ms(5);
    }

    return 0;
}

// time base for self made clock
// clk = 10^6 hz -> 10^(-6) s * 64 * 256 = 0,016384 s
// One tick is 0,016384 s | 61,03515625 hz
ISR(TIM0_OVF_vect)
{
    ticks++;
    OCR0A = leds[0].value;
    OCR0B = leds[1].value;
}

ISR(TIM1_COMPA_vect)
{
    OCR1A = leds[2].value;
}

ISR(TIM1_COMPB_vect)
{
    OCR1B = leds[3].value;
}
