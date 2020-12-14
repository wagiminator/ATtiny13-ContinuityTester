// ATtiny13 Continuity Tester
//
// This code is just a conversion of the original one by David Johnson-Davies
// from the ATtiny85 to the ATtiny13. It implements a simple yet effective
// continuity tester by using the internal analog comparator of the ATtiny.
// Timer0 is set to CTC mode with a TOP value of 149 and a prescaler of 8.
// At a clockspeed of 1.2 MHz it fires every millisecond the compare match A
// interrupt which is used as a simple millis counter. In addition the compare
// match interrupt B can be activated to toggle the buzzer pin at a frequency
// of 1000 Hz.
//
//                          +-\/-+
//        --- A0 (D5) PB5  1|°   |8  Vcc
//        --- A3 (D3) PB3  2|    |7  PB2 (D2) A1 --- LED
// Buzzer --- A2 (D4) PB4  3|    |6  PB1 (D1) ------ Probe
//                    GND  4|    |5  PB0 (D0) ------ Reference
//                          +----+  
//
// Controller:  ATtiny13
// Core:        MicroCore (https://github.com/MCUdude/MicroCore)
// Clockspeed:  1.2 MHz internal
// BOD:         BOD disabled (energy saving)
// Timing:      Micros disabled (Timer0 is in use)
//
// Based on the project by David Johnson-Davies.
// ( http://www.technoblogy.com/show?1YON )
//
// 2020 by Stefan Wagner 
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
// License: http://creativecommons.org/licenses/by-sa/3.0/


// libraries
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// pin definitions
#define REF     PB0
#define PROBE   PB1
#define LED     PB2
#define EMPTY   PB3
#define BUZZER  PB4

// global variables
volatile uint16_t tmillis = 0;                  // counts milliseconds
const    uint16_t timeout = 30000;              // 30 seconds sleep timer

// main function
int main(void) {
  // setup
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);         // set sleep mode to power down
  PRR    = (1<<PRADC);                          // shut down ADC to save power
  DDRB   = (1<<LED) | (1<<BUZZER) | (1<<EMPTY); // LED, BUZZER and EMPTY pin as output
  PORTB  = (1<<LED) | (1<<REF) | (1<<PROBE);    // LED on, internal pullups for REF and PROBE
  OCR0A  = 149;                                 // TOP value for timer0
  OCR0B  = 74;                                  // for generating 1000Hz buzzer tone
  TCCR0A = (1<<WGM01);                          // set timer0 CTC mode
  TCCR0B = (1<<CS01);                           // start timer with prescaler 8
  TIMSK0 = (1<<OCIE0A);                         // enable output compare match A interrupt
  PCMSK  = (1<<PROBE);                          // enable interrupt on PROBE pin
  GIMSK  = (1<<PCIE);                           // enable pin change interrupts
  sei();                                        // enable global interrupts

  // loop
  while(1) {
    if (ACSR & (1<<ACO)) TIMSK0 |=  (1<<OCIE0B);// buzzer on  if comparator output is 1
    else                 TIMSK0 &= ~(1<<OCIE0B);// buzzer off if comparator output is 0
    if (tmillis > timeout) {                    // go to sleep?
      PORTB &= ~(1<<LED);                       // LED off
      PORTB &= ~(1<<REF);                       // turn off pullup to save power
      sleep_mode();                             // go to sleep, wake up by pin change
      PORTB |=  (1<<REF);                       // turn on pullup on REF pin
      PORTB |=  (1<<LED);                       // LED on
    }
  }
}

// pin change interrupt service routine - resets millis
ISR (PCINT0_vect) {
  tmillis = 0;
}

// timer/counter compare match A interrupt service routine (every millisecond)
ISR(TIM0_COMPA_vect) {
  PORTB &= ~(1<<BUZZER);                        // BUZZER pin LOW
  tmillis++;                                    // increase millis counter
}

// timer/counter compare match B interrupt service routine (enabled if buzzer has to beep)
ISR(TIM0_COMPB_vect) {
  PORTB |= (1<<BUZZER);                         // BUZZER pin HIGH
}
