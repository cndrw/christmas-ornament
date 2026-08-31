@echo off

cd firmware\build

avr-gcc -Os -mmcu=attiny84a -Werror ../src/main.c
if errorlevel 1 goto :error_compile

avr-objcopy -O ihex -j .text -j .data a.out a.hex   

avrdude -v -V -p t84a -c stk500v1 -PCOM6 -b19200 -U flash:w:a.hex:i
if errorlevel 1 goto :error_flash

exit /b 0

:error_compile
echo Compile Error
exit /b 1

:error_flash
echo Flashing Failed
exit /b 1