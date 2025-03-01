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
#define ATT_CALL 6
#define RF_SYNC 7

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
volatile uint8_t pinb;     // last PINB state
uint32_t incoming; // rf scrach data
uint8_t read;      // track num read bits
data_t current;    // last packet received

uint8_t EEMEM EEPROM_PEER = 0;

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
    if (bit_is_set(flags, ACTIVE))
    {
        sbi(flags, RX_READY);
        pinb = PINB;
    }
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
        pair = eeprom_read_byte(EEPROM_PEER);
    }
    while (1)
    {
      
    }
}