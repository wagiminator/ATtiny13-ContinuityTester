// ===================================================================================
// Project:   Continuity Tester based on ATtiny13A
// Version:   v1.1
// Year:      2020
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// This code is just a conversion of the original one by David Johnson-Davies
// from the ATtiny85 to the ATtiny13A. It implements a simple yet effective
// continuity tester by using the internal analog comparator of the ATtiny.
// Timer0 is set to CTC mode with a TOP value of 127 and no prescaler.
// At a clockspeed of 128 kHz it fires every millisecond the compare match A
// interrupt which is used as a simple millis counter. In addition the compare
// match interrupt B can be activated to toggle the buzzer pin at a frequency
// of 1000 Hz.
//
// References:
// -----------
// Based on the project by David Johnson-Davies:
// http://www.technoblogy.com/show?1YON
//
// Buzzer enhancements committed by Qi Wenmin:
// https://github.com/qiwenmin
//
// Wiring:
// -------
//                           +-\/-+
//        --- RST ADC0 PB5  1|°   |8  Vcc
//        ------- ADC3 PB3  2|    |7  PB2 ADC1 -------- LED
// Buzzer ------- ADC2 PB4  3|    |6  PB1 AIN1 OC0B --- Probe
//                     GND  4|    |5  PB0 AIN0 OC0A --- Reference
//                           +----+
//
// Compilation Settings:
// ---------------------
// Controller:  ATtiny13A
// Core:        MicroCore (https://github.com/MCUdude/MicroCore)
// Clockspeed:  128 kHz internal
// BOD:         BOD disabled
// Timing:      Micros disabled
//
// Leave the rest on default settings. Don't forget to "Burn bootloader"!
// No Arduino core functions or libraries are used. Use the makefile if 
// you want to compile without Arduino IDE.
//
// Fuse settings: -U lfuse:w:0x3b:m -U hfuse:w:0xff:m


// ===================================================================================
// Libraries and Definitions
// ===================================================================================

// Libraries
#include <avr/io.h>                             // for GPIO
#include <avr/sleep.h>                          // for sleep mode
#include <avr/interrupt.h>                      // for interrupts

// Pin definitions
#define REF       PB0                           // reference pin
#define PROBE     PB1                           // pin connected to probe
#define LED       PB2                           // pin connected to LED
#define EMPTY     PB3                           // unused pin
#define BUZZER    PB4                           // pin connected to buzzer

// Firmware parameters
#define TIMEOUT   10000                         // sleep timer in ms
#define DEBOUNCE  50                            // buzzer debounce time in ms

// Global variables
volatile uint16_t tmillis = 0;                  // counts milliseconds

// ===================================================================================
// Main Function
// ===================================================================================

int main(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          // set sleep mode to power down
  PRR    = (1<<PRADC);                          // shut down ADC to save power
  DIDR0  = (1<<REF) | (1<<EMPTY);               // disable digital input on REF and EMPTY
  DDRB   = (1<<LED) | (1<<BUZZER);              // LED and BUZZER pin as output
  PORTB  = (1<<LED) | (1<<REF) | (1<<PROBE);    // LED on, internal pullups for REF and PROBE
  OCR0A  = 127;                                 // TOP value for timer0
  OCR0B  = 63;                                  // for generating 1000Hz buzzer tone
  TCCR0A = (1<<WGM01);                          // set timer0 CTC mode
  TCCR0B = (1<<CS00);                           // start timer with no prescaler
  TIMSK0 = (1<<OCIE0A);                         // enable output compare match A interrupt
  PCMSK  = (1<<PROBE);                          // enable interrupt on PROBE pin
  sei();                                        // enable global interrupts

  // Loop
  while(1) {
    if(ACSR & (1<<ACO)) {                       // continuity detected?
      tmillis = 0;                              // reset millis counter
      TIMSK0 |= (1<<OCIE0B);                    // buzzer on
    } else if(tmillis > DEBOUNCE)               // no continuity detected?
      TIMSK0 &= ~(1<<OCIE0B);                   // buzzer off after debounce time

    if(tmillis > TIMEOUT) {                     // go to sleep?
      PORTB &= ~(1<<LED);                       // LED off
      PORTB &= ~(1<<REF);                       // turn off pullup to save power
      GIMSK  =  (1<<PCIE);                      // enable pin change interrupts
      sleep_mode();                             // go to sleep, wake up by pin change
      GIMSK  = 0;                               // disable pin change interrupts
      PORTB |=  (1<<REF);                       // turn on pullup on REF pin
      PORTB |=  (1<<LED);                       // LED on
    }
  }
}

// ===================================================================================
// Interrupt Service Routines
// ===================================================================================

// Pin change interrupt service routine - resets millis
ISR(PCINT0_vect) {
  tmillis = 0;                                  // reset millis counter
}

// Timer/counter compare match A interrupt service routine (every millisecond)
ISR(TIM0_COMPA_vect) {
  PORTB &= ~(1<<BUZZER);                        // BUZZER pin LOW
  tmillis++;                                    // increase millis counter
}

// Timer/counter compare match B interrupt service routine (enabled if buzzer has to beep)
ISR(TIM0_COMPB_vect) {
  PORTB |= (1<<BUZZER);                         // BUZZER pin HIGH
}
