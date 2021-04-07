rem MM8Dtiny v0.1 * Central controlling device
rem Copyright (C) 2021 Pozsar Zsolt pozsar.zsolt@szerafingomba.hu
rem sc_com1.bat
rem Redirect output to serial console

@set CONSOLE=COM1:
@echo Output is redirected to %CONSOLE% serial port.
@mode %CONSOLE% 9600,n,8,1,,
@mm8dtiny --double > %CONSOLE%
