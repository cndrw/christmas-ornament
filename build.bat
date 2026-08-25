@echo off

cd firmware\build
avr-gcc -Os -mmcu=attiny84a ../src/main.c
avr-objcopy -O ihex -j .text -j .data a.out a.hex   
avrdude -v -V -p t84a -c stk500v1 -PCOM6 -b19200 -U flash:w:a.hex:i