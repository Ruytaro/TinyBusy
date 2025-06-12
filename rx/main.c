#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define PROG_PIN PB4
#define TX_PIN PB2
#define RX_PIN PB1
#define LED_PIN PB0

#define HIGH_TIME 12
#define LOW_TIME 5

#define MAX_COUNTER 24

#define ATT_PULSES 4
typedef union data
{
 struct
{
    uint8_t red : 5;
    uint8_t green : 6;
    uint8_t blue : 5;
    uint8_t code : 7;
    uint8_t parity : 1;
} ;
uint32_t raw; // raw data for easy access
} data_t;

typedef union rgb
{
    uint32_t raw;
    struct main
{
    uint8_t red;    // 5 bits
    uint8_t green;  // 6 bits
    uint8_t blue;   // 5 bits
}dis;
} rgb_t;

volatile data_t rx_data; // data received from RF
volatile uint8_t bit;
volatile uint8_t bit_count; // number of bits received

void updateLED();
void enableRX();

rgb_t current;    // last packet received

ISR(PCINT0_vect)
{
    //PCInt
}

ISR(WDT_vect)
{
    updateLED();
}

ISR(INT0_vect)
{
    //Int0 (RF_RX pin)
}

ISR(TIM0_COMPA_vect)
{
    PORTB |= _BV(LED_PIN);
    current.raw >> (23 - bit) & 1 ? OCR0B = HIGH_TIME : OCR0B = LOW_TIME;
    bit++;
}

ISR(TIM0_COMPB_vect)
{
    PORTB &= ~_BV(LED_PIN);
    if (bit > 23){
        enableRX();
    }
}

void enableRX()
{
    GIMSK |= _BV(INT0);                     // enable INT0
    TIMSK0 &= ~(_BV(OCIE0A) | _BV(OCIE0B)); // disable Timer Compare interrupts A and B
    bit = 0;                                 // reset bit counter
}

void updateLED(){
    bit=0;
    GIMSK &= ~_BV(INT0);                    // disable INT0
    OCR0A = MAX_COUNTER;                    // set max counter value (9.6MHz/24=>400kHz)
    OCR0B = MAX_COUNTER;                    // set threshold counter
    TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B);    // enable Timer Compare interrupts A and B
}



int main(void)
{
    DDRB = _BV(LED_PIN) | _BV(TX_PIN); // set LED pins as OUTPUT
    PORTB = _BV(PROG_PIN)|_BV(RX_PIN);  // set PROG_PIN pull-up
    cbi(ADCSRA, ADEN);                                    // disable ADC
    sbi(ACSR, ACD);                                       // disable Analog comparator
    TCCR0A |= _BV(WGM01);
    TCCR0B |= _BV(CS00);                                  // set prescaler to 1 (CLK=9.600.000/1=>9.6MHz)
    WDTCR = _BV(WDTIE)| _BV(WDP1);                          // enable WDT interrupt with 64ms timeout          

    current.dis.red = 0x1F;                                    // set demo data for testing
    sei();                                                // enable global interrupts
    
    while (1)
    {
        
    }
}