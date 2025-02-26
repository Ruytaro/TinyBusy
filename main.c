#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define PROG_PIN PB4
#define RX_PIN PB3
#define BLUE_LED PB2
#define GREEN_LED PB1
#define RED_LED PB0

#define PAIRED 0
#define COLLISION 1
#define ACTIVE 2
#define BUTTON 3
#define RX_READY 4
#define RX_READ 5

#define MAX_COUNTER 20

#define ATT_PULSES 8

typedef struct data
{
    uint8_t pair : 8;
    uint8_t ipair : 8;
    uint8_t red : 5;
    uint8_t green : 5;
    uint8_t blue : 5;
    uint8_t parity : 1;
} data_t;

volatile uint8_t ticks;    // system ticks for soft-pwm
volatile uint8_t pair;     // transmitter pairing code
volatile uint8_t rf_ticks; // rf ticks
volatile uint8_t flags;    // global status flags
uint32_t incoming; // rf scrach data
uint8_t read;      // track num read bits
data_t current;    // last packet received

uint8_t EEMEM EEPROM_PEER = 0;

ISR(PCINT0_vect)
{
    rf_ticks++;
}

ISR(TIM0_COMPB_vect)
{
    if (bit_is_set(flags, ACTIVE))
    {
        sbi(flags, RX_READY);
        bit_is_set(PINB, RX_PIN) ? sbi(flags, RX_READ) : cbi(flags, RX_READ);
    }
}

ISR(TIM0_COMPA_vect)
{
    ticks++;
    cbi(flags, COLLISION);
    if (rf_ticks > ATT_PULSES)
    {
        if (bit_is_set(flags, ACTIVE))
        {
            sbi(flags, COLLISION);
        }
        else
        {
            sbi(flags, ACTIVE);
        }
    }
    rf_ticks = 0;
    if (sbi(flags, COLLISION))
    {
        cbi(flags, ACTIVE);
    }
}

void rxRead()
{
    cbi(flags, RX_READY);
    if (bit_is_set(flags, RX_READ))
    {
        incoming |= 1 << read;
    }
    read++;
    if (read == 32)
    {
        data_t *data = (data_t *)&incoming;
        if (!data->pair ^ data->ipair)
        {
            if (data->pair == pair)
            {
                current = *data;
            }
        }
        read = 0;
        incoming = 0;
    }
}

void softpwm(uint8_t value)
{
    if (current.red < value)
    {
        sbi(PORTB, RED_LED);
    }
    else
    {
        cbi(PORTB, RED_LED);
    }
    if (current.green < value)
    {
        sbi(PORTB, GREEN_LED);
    }
    else
    {
        cbi(PORTB, GREEN_LED);
    }
    if (current.blue < value)
    {
        sbi(PORTB, BLUE_LED);
    }
    else
    {
        cbi(PORTB, BLUE_LED);
    }
}

int main(void)
{
    DDRB = _BV(RED_LED) | _BV(GREEN_LED) | _BV(BLUE_LED); // set LED pins as OUTPUT
    PORTB = _BV(PROG_PIN);                                // set PROG_PIN pull-up
    cbi(ADCSRA, ADEN);                                    // disable ADC
    sbi(ACSR, ACD);                                       // disable Analog comparator
    TCCR0B |= _BV(CS01);                                  // set prescaler to 8 (CLK=1.200.000/8=>150kHz)
    OCR0A = MAX_COUNTER;                                  // set max counter value (150kHz/100=>1.5kHz)
    OCR0B = MAX_COUNTER / 2;                              // set threshold counter
    TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B);                  // enable Timer Compare interrupts A and B
    sbi(GIMSK, PCIE);                                     // enable PinChangeInterrupts
    PCMSK |= _BV(RX_PIN) | _BV(PROG_PIN);                 // set mask for PCI
    current.red = 0x1f;                                   // set demo data for testing
    sei();                                                // enable global interrupts
    if (bit_is_set(PINB, PROG_PIN))
    {
        sbi(flags, PAIRED);
        pair = eeprom_read_byte(EEPROM_PEER);
    }
    while (1)
    {
        softpwm(ticks & 0x1F);
        if (bit_is_set(flags, RX_READY))
            rxRead();
    }
}