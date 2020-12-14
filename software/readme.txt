avrdude -c usbasp -p t13 -U lfuse:w:0x2a:m -U hfuse:w:0xfb:m -U flash:w:ContinuityTester.hex
