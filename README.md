# Continuity Tester based on ATtiny13A
The simple yet effective Continuity Tester is just a conversion of the original one by [David Johnson-Davies](http://www.technoblogy.com/show?1YON) from the ATtiny85 to the ATtiny13A. It is designed to check circuit wiring and PCB tracks.

- Design Files (EasyEDA): https://easyeda.com/wagiminator/attiny13-continuity-tester

![pic1.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-ContinuityTester/main/documentation/ContinuityTester_pic1.jpg)

# Hardware
The basic wiring is shown below:

![wiring.png](https://raw.githubusercontent.com/wagiminator/ATtiny13-ContinuityTester/main/documentation/ContinuityTester_wiring.png)

Connect one end of a wire to the GND terminal and use the other end together with the pogo pin to check the continuity of wires and traces. The device is powered by a 1220 coin cell battery. Please remember that only the rechargeable LIR1220 Li-Ion batteries work. The "normal" CR1220s don't deliver enough power for the buzzer.

# Software
## Implementation
The code is using the internal analog comparator of the ATtiny. By using the internal pullup resistors on both inputs of the comparator and by using a 51 Ohm pulldown resistor to form a voltage divider on the positive input, the comparator output becomes high if the resistance between both probes is less then 51 Ohms. This indicates a continuity between the probes and the buzzer will be turned on. For a more precise explanation refer to [David's project](http://www.technoblogy.com/show?1YON). Timer0 is set to CTC mode with a TOP value of 127 and no prescaler. At a clockspeed of 128 kHz it fires every millisecond the compare match A interrupt which is used as a simple millis counter. In addition the compare match interrupt B can be activated to toggle the buzzer pin at a frequency of 1000 Hz, which creates a "beep". If no continuity between the probes is detected for 30 seconds, the ATtiny is put into sleep, consuming almost no power. The device can be reactivated by holding the two probes together. The LED lights up when the device is activated and goes out when the ATtiny is asleep. The code needs only 252 bytes of flash if compiled with LTO.

```c
// Libraries
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Pin definitions
#define REF     PB0
#define PROBE   PB1
#define LED     PB2
#define EMPTY   PB3
#define BUZZER  PB4

// Global variables
volatile uint16_t tmillis = 0;                  // counts milliseconds
const    uint16_t timeout = 30000;              // 30 seconds sleep timer

// Main Function
int main(void) {
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);         // set sleep mode to power down
  PRR    = (1<<PRADC);                          // shut down ADC to save power
  DDRB   = (1<<LED) | (1<<BUZZER) | (1<<EMPTY); // LED, BUZZER and EMPTY pin as output
  PORTB  = (1<<LED) | (1<<REF) | (1<<PROBE);    // LED on, internal pullups for REF and PROBE
  OCR0A  = 127;                                 // TOP value for timer0
  OCR0B  = 63;                                  // for generating 1000Hz buzzer tone
  TCCR0A = (1<<WGM01);                          // set timer0 CTC mode
  TCCR0B = (1<<CS00);                           // start timer with no prescaler
  TIMSK0 = (1<<OCIE0A);                         // enable output compare match A interrupt
  PCMSK  = (1<<PROBE);                          // enable interrupt on PROBE pin
  GIMSK  = (1<<PCIE);                           // enable pin change interrupts
  sei();                                        // enable global interrupts

  // Loop
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

// Pin change interrupt service routine - resets millis
ISR (PCINT0_vect) {
  tmillis = 0;
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
```

## Compiling and Uploading
Since there is no ICSP header on the board, you have to program the ATtiny either before soldering using an [SOP adapter](https://aliexpress.com/wholesale?SearchText=sop-8+150mil+adapter), or after soldering using an [EEPROM clip](https://aliexpress.com/wholesale?SearchText=sop8+eeprom+programming+clip). The [AVR Programmer Adapter](https://github.com/wagiminator/AVR-Programmer/tree/master/AVR_Programmer_Adapter) can help with this.

### If using the Arduino IDE
- Make sure you have installed [MicroCore](https://github.com/MCUdude/MicroCore).
- Go to **Tools -> Board -> MicroCore** and select **ATtiny13**.
- Go to **Tools** and choose the following board options:
  - **Clock:**  128 kHz internal osc.
  - **BOD:**    BOD disabled
  - **Timing:** Micros disabled
- Connect your programmer to your PC and to the ATtiny.
- Go to **Tools -> Programmer** and select your ISP programmer (e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp)).
- Go to **Tools -> Burn Bootloader** to burn the fuses.
- Open ContinuityTester.ino and click **Upload**.

### If using the precompiled hex-file
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Connect your programmer to your PC and to the ATtiny.
- Open a terminal.
- Navigate to the folder with the hex-file.
- Execute the following command (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p t13 -U lfuse:w:0x3b:m -U hfuse:w:0xff:m -U flash:w:continuitytester.hex
  ```

### If using the makefile (Linux/Mac)
- Make sure you have installed [avr-gcc toolchain and avrdude](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/).
- Connect your programmer to your PC and to the ATtiny.
- Open the makefile and change the programmer if you are not using usbasp.
- Open a terminal.
- Navigate to the folder with the makefile and sketch.
- Run "make install" to compile, burn the fuses and upload the firmware.

# References, Links and Notes
1. [Original Project by David Johnson-Davies](http://www.technoblogy.com/show?1YON)
2. [ATtiny13A Datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/doc8126.pdf)

![pic2.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-ContinuityTester/main/documentation/ContinuityTester_pic2.jpg)
![pic3.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-ContinuityTester/main/documentation/ContinuityTester_pic3.jpg)
