// ===================================================================================
// Project:   Continuity Tester based on ATtiny13A
// Version:   v1.0
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
#define REF     PB0                             // reference pin
#define PROBE   PB1                             // pin connected to probe
#define LED     PB2                             // pin connected to LED
#define EMPTY   PB3                             // unused pin
#define BUZZER  PB4                             // pin connected to buzzer

// Global variables
volatile uint8_t t_in_50millis = 0;             // counts within 50 milliseconds
volatile uint8_t t50millis = 0;                 // counts of 50 milliseconds
const    uint8_t timeout_50ms = 200;            // 10 seconds sleep timer

static const uint8_t debounce_timeout_50ms = 1; // delay 50ms before turning buzzer off

// ===================================================================================
// Main Function
// ===================================================================================

int main(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          // set sleep mode to power down
  PRR    = (1<<PRADC);                          // shut down ADC to save power
  DDRB   = (1<<LED) | (1<<BUZZER) | (1<<EMPTY); // LED, BUZZER and EMPTY pin as output
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
    if (ACSR & (1<<ACO)) {
      t_in_50millis = 0;                        // reset millis counters
      t50millis = 0;
      TIMSK0 |=  (1<<OCIE0B);                   // buzzer on  if comparator output is 1
    } else {
      if (t50millis > debounce_timeout_50ms) {
        TIMSK0 &= ~(1<<OCIE0B);                 // buzzer off if comparator output is 0
      }
    }
    if (t50millis > timeout_50ms) {             // go to sleep?
      PORTB &= ~(1<<LED);                       // LED off
      PORTB &= ~(1<<REF);                       // turn off pullup to save power
      GIMSK = (1<<PCIE);                        // enable pin change interrupts
      sleep_mode();                             // go to sleep, wake up by pin change
      GIMSK &= ~(1<<PCIE);                      // disable pin change interrupts
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
  t_in_50millis = 0;                            // reset millis counters
  t50millis = 0;
}

// Timer/counter compare match A interrupt service routine (every millisecond)
ISR(TIM0_COMPA_vect) {
  PORTB &= ~(1<<BUZZER);                        // BUZZER pin LOW
  t_in_50millis++;                              // increase millis countera
  if (t_in_50millis == 50) {
    t_in_50millis = 0;
    t50millis++;
  }
}

// Timer/counter compare match B interrupt service routine (enabled if buzzer has to beep)
ISR(TIM0_COMPB_vect) {
  PORTB |= (1<<BUZZER);                         // BUZZER pin HIGH
}
