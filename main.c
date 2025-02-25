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
#define SOFT_PWM 3

#define MAX_COUNTER 100

#define ATT_PULSES 8

typedef struct data
{
    uint8_t pair : 8;
    uint8_t red : 5;
    uint8_t green : 5;
    uint8_t blue : 5;
    uint8_t parity : 1;
} data_t;

volatile data_t current;    // last packet received
volatile uint8_t ticks;     // system ticks for soft-pwm
volatile uint8_t pair;      // transmitter pairing code
volatile uint16_t incoming; // rf scrach data
volatile uint8_t read;      // track num read bits
volatile uint8_t rf_ticks;  // rf ticks
volatile uint8_t flags;     // global status flags
volatile uint8_t button_flag;

uint8_t EEMEM EEPROM_PEER = 0;

ISR(WDT_vect){
    button_flag = (button_flag << 1) | bit_is_set(PINB, PROG_PIN);
    if (button_flag == 0)
    {  

    }
      
}

ISR(PCINT0_vect)
{
        rf_ticks++;
}

ISR(TIM0_COMPB_vect)
{
    if (bit_is_clear(flags, ACTIVE))
        return;
    incoming = ((incoming << 1) | bit_is_set(PINB, RX_PIN));
    read++;
    if (read == 8 && bit_is_set(flags, PAIRED) && incoming != pair)
        cbi(flags, ACTIVE);
    if (read == 16)
    {
        current = *(data_t *)incoming;
        cbi(flags, ACTIVE);
        read = 0;
    }
    if (bit_is_clear(flags, PAIRED) && (incoming & 0xFF) == (incoming >> 8))
    {
        eeprom_update_byte(EEPROM_PEER, incoming >> 8);
        pair = incoming >> 8;
        sbi(flags, PAIRED);
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
    uint8_t value = ticks%32;
    if (current.red > value)
    {
        cbi(PORTB, RED_LED);
    }
    else
    {
        sbi(PORTB, RED_LED);
    }    
    if (current.green > value)
    {
        cbi(PORTB, GREEN_LED);
    }
    else
    {
        sbi(PORTB, GREEN_LED);
    }
    if (current.blue > value)
    {
        cbi(PORTB, BLUE_LED);
    }
    else
    {
        sbi(PORTB, BLUE_LED);
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
    PCMSK |= _BV(RX_PIN);                                 // set mask for PCI
    wdt_enable(WDTO_120MS);                               // set prescaler to 0.120s and enable Watchdog Timer
    WDTCR |= _BV(WDTIE);                                  // enable Watchdog Timer interrupt
    sei();                                                // enable global interrupts
    if (bit_is_set(PINB, PROG_PIN))
    {
        sbi(flags, PAIRED);
        pair = eeprom_read_byte(EEPROM_PEER);
    }
    while (1){
    }
}