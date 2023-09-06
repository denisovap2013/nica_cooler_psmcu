# nica_cooler_psmcu
Server and client applications for the set of power source control units

## Requirements

* Lab Windows CVI 9.0
* Windows 10
* cangw.lib, cangw.dll (hosted by BINP, Russia)

`cangw.lib` and `cangw.dll` files must be placed into the folder `PS-MCU server`.

## Server commands

* `PSMCU:SINGLE:FULLINFO [deviceIndex]`
* `PSMCU:SINGLE:DAC:SET [deviceIndex] [dacChannel] [value]`
* `PSMCU:SINGLE:DAC:RAWSET [deviceIndex] [dacChannel] [voltage]`
* `PSMCU:SINGLE:ADC:GET [deviceIndex] [channel]`
* `PSMCU:SINGLE:ADC:RAWGET [deviceIndex] [channel]`
* `PSMCU:SINGLE:ADC:RESET [deviceIndex]`
* `PSMCU:SINGLE:ALLREGS:GET [deviceIndex]`
* `PSMCU:SINGLE:OUTREGS:SET [deviceIndex] [val] [mask]`
* `PSMCU:SINGLE:INTERLOCK:DROP [deviceIndex]`
* `PSMCU:SINGLE:INTERLOCK:RESTORE [deviceIndex]`
* `PSMCU:SINGLE:FORCE:ON [deviceIndex]`
* `PSMCU:SINGLE:FORCE:OFF [deviceIndex]`
* `PSMCU:SINGLE:PERMISSION:ON [deviceIndex]`
* `PSMCU:SINGLE:PERMISSION:OFF [deviceIndex]`
* `PSMCU:SINGLE:STATUS:GET [deviceIndex]`
* `PSMCU:SINGLE:DEVNAME:GET [deviceIndex]`
* `PSMCU:SINGLE:ADCNAME:GET [deviceIndex] [adcChannel]`
* `PSMCU:SINGLE:DACNAME:GET [deviceIndex] [dacChannel]`
* `PSMCU:SINGLE:INREGNAME:GET [deviceIndex] [inRegChannel]`
* `PSMCU:SINGLE:OUTREGNAME:GET [deviceIndex] [outRegChannel]`
* `PSMCU:ALL:ADC:RESET`
* `PSMCU:ALL:INTERLOCK:DROP`
* `PSMCU:ALL:INTERLOCK:RESTORE`
* `PSMCU:ALL:FORCE:ON`
* `PSMCU:ALL:FORCE:OFF`
* `PSMCU:ALL:PERMISSION:ON`
* `PSMCU:ALL:PERMISSION:OFF`
* `PSMCU:CANGW:STATUS:GET`
