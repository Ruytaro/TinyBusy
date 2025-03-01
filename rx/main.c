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
#define RX_READY 3
#define ATT_CALL 4
#define RF_SYNC 5

#define MAX_COUNTER 20

#define ATT_PULSES 8

typedef struct data
{
    uint8_t pair : 8;
    uint8_t red : 5;
    uint8_t green : 5;
    uint8_t blue : 5;
    uint8_t parity : 1;
    uint8_t ipair : 8;
} data_t;

typedef struct counter
{
    uint8_t read : 7;
    uint8_t parity : 1;
} counter_t;

volatile uint8_t ticks;    // system ticks for soft-pwm
volatile uint8_t pair;     // transmitter pairing code
volatile uint8_t rf_ticks; // rf ticks
volatile uint8_t flags;    // global status flags
volatile uint8_t pinb;     // last PINB state
uint32_t incoming; // rf scrach data
counter_t counter; // track num read bits
data_t current;    // last packet received

uint8_t EEMEM EEPROM_PEER = 0xAA;

ISR(PCINT0_vect)
{
    rf_ticks++;
    if (rf_ticks > ATT_PULSES)
        sbi(flags,ATT_CALL);
    if (bit_is_clear(PINB,RX_PIN)&&bit_is_set(flags,RF_SYNC))
    TCNT0 = 0;
}

ISR(TIM0_COMPA_vect)
{
    ticks++;
    rf_ticks = 0;
    if (flags & (_BV(ATT_CALL)|_BV(ACTIVE))){
        sbi(flags,COLLISION);
    }
    cbi(flags, ATT_CALL);
}

ISR(TIM0_COMPB_vect)
{
    sbi(flags,RF_SYNC);
    pinb = PINB;
    if (bit_is_set(flags, ACTIVE))
        sbi(flags, RX_READY);
    
}

void rxRead()
{
    cbi(flags, RX_READY);
    if (bit_is_set(pinb,RX_PIN)){
        incoming |= 1 << counter.read;
        counter.parity++;
    }
    counter.read++;
    if (counter.read == 32)
    {
        data_t *data = (data_t *)&incoming;
        if (data->pair == (uint8_t)~data->ipair && data->parity==counter.parity)
        {
            if (data->pair == pair)
            {
                current = *data;
            }
        }
        counter.read = 0;
        incoming = 0;
    }
}

void softpwm(uint8_t value){
    uint8_t mask = 0x0;
    if (current.red <= value)
    {
        mask |= _BV(RED_LED);
    }
    if (current.green <= value)
    {
        mask |= _BV(GREEN_LED);
    }
    if (current.blue <= value)
    {
        mask |= _BV(BLUE_LED);
    }
    PORTB |= mask;
}

int main(void)
{
    DDRB = _BV(RED_LED) | _BV(GREEN_LED) | _BV(BLUE_LED); // set LED pins as OUTPUT
    PORTB = _BV(PROG_PIN);                                // set PROG_PIN pull-up
    cbi(ADCSRA, ADEN);                                    // disable ADC
    sbi(ACSR, ACD);                                       // disable Analog comparator
    TCCR0B |= _BV(CS01);                                  // set prescaler to 8 (CLK=9.600.000/8=>1.2MHz)
    OCR0A = MAX_COUNTER;                                  // set max counter value (1.2MHz/20=>60kHz)
    OCR0B = MAX_COUNTER / 2;                              // set threshold counter
    TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B);                  // enable Timer Compare interrupts A and B
    sbi(GIMSK, PCIE);                                     // enable PinChangeInterrupts
    PCMSK |= _BV(RX_PIN) | _BV(PROG_PIN);                 // set mask for PCI
    current.red = 0x1f;                                   // set demo data for testing
    sei();                                                // enable global interrupts
    if (bit_is_set(PINB, PROG_PIN))
    {
        sbi(flags, PAIRED);
        pair = eeprom_read_byte(&EEPROM_PEER);
    }
    while (1)
    {
        softpwm(ticks & 0x1F);
        if (bit_is_set(flags,COLLISION)){
            counter.read=0;
            incoming=0;
            cbi(flags,COLLISION);
            cbi(flags,ACTIVE);
        }
        if (bit_is_set(flags, RX_READY))
            rxRead();
    }
}