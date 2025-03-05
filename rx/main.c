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
volatile uint8_t pinb;     // last PINB state

volatile uint8_t rf_sync;
volatile uint8_t att_call;
volatile uint8_t active;
volatile uint8_t rx_ready;
volatile uint8_t collision;
volatile uint8_t paired;

uint32_t incoming; // rf scrach data
counter_t counter; // track num read bits
data_t current;    // last packet received

uint8_t EEMEM EEPROM_PEER = 0xAA;

ISR(PCINT0_vect)
{
    rf_ticks++;
    if (rf_ticks > ATT_PULSES)
        att_call = 1;
    if (bit_is_set(PINB,RX_PIN)&&rf_sync){
        TCNT0 = 0;
        rf_sync = 0;
    }
}

ISR(TIM0_COMPA_vect)
{
    ticks++;
    rf_ticks = 0;
    if (att_call && paired)
        collision=1;
    att_call=0;
}

ISR(TIM0_COMPB_vect)
{
    rf_sync=1;
    pinb = PINB;
    if (active)
        rx_ready = 1;    
}

void rxRead()
{
    rx_ready = 0;
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
    uint8_t mask = _BV(PROG_PIN);
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
    PORTB = mask;
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
    current.red = 0xF;                                   // set demo data for testing
    sei();                                                // enable global interrupts
    if (bit_is_set(PINB, PROG_PIN))
        paired = 1;
    pair = eeprom_read_byte(&EEPROM_PEER);
    
    while (1)
    {
        softpwm(ticks & 0x1F);
        if (rx_ready)
            rxRead();
        if (collision){
            counter.read=0;
            incoming=0;
            collision=0;
            active=0;
        }
    }
}