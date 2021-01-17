avrdude -c usbasp -p t13 -U lfuse:w:0x3b:m -U hfuse:w:0xff:m -U flash:w:ContinuityTester.hex
