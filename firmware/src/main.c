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

#define N_LED 4

#define LED_EFFECT_SEMI_CYCLE_MIN_BRIGHTNESS 107

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;

typedef struct {
    void(*init)(void);
    void(*update)(void);
}led_effect_t;

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
    { .led = &leds[0], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 800 },
    { .led = &leds[1], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 800 },
    { .led = &leds[2], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 800 },
    { .led = &leds[3], .enabled = 0, .t = 0, .direction = 1, .delay_ms = 800 }
};

sparkle_blink_t* possible_selection[3] = {0};
u8 schedule_lock = 0;
sparkle_blink_t* active_led = &sparke_blinks[0];
u64 last_update;

void noop(void) {}
void sparkle_effect_semi_cycle_init(void);
void sparkle_effect_full_cycle(void);
void sparkle_effect_semi_cycle(void);
void alternate_effect(void);

const led_effect_t led_effects[] = {
    { .init = noop, .update = sparkle_effect_full_cycle },
    { .init = sparkle_effect_semi_cycle_init, .update = sparkle_effect_semi_cycle },
    { .init = noop, .update = alternate_effect }
};

u8 cur_effect = 2;

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

u8 led_blink(sparkle_blink_t* const handle, const u8 min_value)
{
    handle->led->value = ease_in_cubic(handle->t);

    if (handle->t == UINT8_MAX) handle->direction = -1;
    if (handle->t == min_value) handle->direction = 1;

    handle->t += handle->direction;

    return handle->t == min_value;
}

u8 select_disabled_leds(sparkle_blink_t* const leds, const u8 size, sparkle_blink_t** out)
{
    u8 count = 0;
    for (u8 i = 0; i < size; i++)
    {
        if (!leds[i].enabled)
        {
            out[count] = &leds[i];
            count++;
        }
    }

    return count;
}

void sparkle_effect(const u8 min_brightness)
{
    if (!active_led->enabled && get_time_ms() - last_update > active_led->delay_ms)
    {
        active_led->enabled = 1;

        const u8 num_possible_selections = select_disabled_leds(sparke_blinks, N_LED, possible_selection);
        if (num_possible_selections > 0)
        {
            const u8 selected_led_idx = xorshift32() % num_possible_selections;
            active_led = possible_selection[selected_led_idx];
            // active_led->delay_ms = (xorshift32() % 2305) + 1050;
            active_led->delay_ms = (xorshift32() % 1800) + 1050;
        }
        else
        {
            schedule_lock = 1;
        }

        last_update = get_time_ms();
    }


    for (u8 i = 0; i < N_LED; i++)
    {
        if(sparke_blinks[i].enabled)
        {
            if (led_blink(&sparke_blinks[i], min_brightness))
            {
                sparke_blinks[i].enabled = 0;
                if (schedule_lock)
                {
                    active_led = &sparke_blinks[i];
                    schedule_lock = 0;
                }
            }
        }
    }
}

void sparkle_effect_full_cycle()
{
    sparkle_effect(0);
}

void sparkle_effect_semi_cycle_init()
{
    for (u8 i = 0; i < N_LED; i++)
    {
        sparke_blinks[i].t = LED_EFFECT_SEMI_CYCLE_MIN_BRIGHTNESS;
        leds[i].value = ease_in_cubic(LED_EFFECT_SEMI_CYCLE_MIN_BRIGHTNESS);
    }
}

void sparkle_effect_semi_cycle()
{
    sparkle_effect(LED_EFFECT_SEMI_CYCLE_MIN_BRIGHTNESS);
}

void alternate_effect(void)
{
    static u8 parity = 0;

    for (u8 i = 0; i < N_LED; i++)
    {
        leds[i].value = (i % 2 == parity) * UINT8_MAX;
    }

    parity = !parity;
    _delay_ms(600);
}

void handle_mode_switching(void)
{
    static u64 last_update = 0;
    static u8 cur_state = 0;
    static u8 last_state = 0;
    static u8 allow_btn_press = 1;

    cur_state = (PINA & (1 << PA1)) == 0;

    if (get_time_ms() - last_update > 400)
    {
        allow_btn_press = 1;
        last_update = get_time_ms();
    }

    if (allow_btn_press && cur_state == 1 && last_state == 0)
    {
        cur_effect = (cur_effect + 1) % 3;
        allow_btn_press = 0;
    }

    last_state = cur_state;
}

int main(void)
{
    DDRB |= (1 << PB2);
    DDRA |= (1 << PA5) | (1 << PA6) | (1 << PA7);
    PORTA |= (1 << PA1); // input pullup

    setup_timer0();
    setup_timer1();

    sei();  // enable interrupt

    last_update = get_time_ms();
    led_effects[cur_effect].init();
    while (1)
    {
        handle_mode_switching();
        led_effects[cur_effect].update();
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

// TODO:
// - für semi_cycle effect können die timings angepasst werden, da der zyklus kürzer ist