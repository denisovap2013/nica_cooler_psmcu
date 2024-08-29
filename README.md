# nica_cooler_psmcu
Server and client applications for the set of power source control units

## Requirements

* Lab Windows CVI 9.0
* Windows 10
* cangw.lib, cangw.dll (hosted by BINP, Russia)

## Server commands

### Data acquisition and registers control

<details>
<summary><code>PSMCU:SINGLE:FULLINFO [deviceIndex]</code></summary>
Provides the information aboud the device.

The answer has the following form:
`PSMCU:SINGLE:FULLINFO [deviceIndex] [adc_ch_0] .. [adc_ch_4] [dac_ch_0] [input_registers_hex] [output_registers_hex] [status_hex]`

* `[adc_ch_k]` - measurements from the k-th ADC channel
* `[dac_ch_0]` - the setup DAC value.
* `[input_registers_hex]` - the state of the input registers
* `[output_registers_hex]` - the state of the output registers
* `[status_hex]` - the status of the device. 1st bit - alive status, 2nd bit - error status.

> Returned values from the DAC and ADC channels are not raw but processed with the linear transformation a*x+b, defined in the server's configuration file.
</details>

<details><summary><code>PSMCU:SINGLE:DAC:SET [deviceIndex] [dacChannel] [value]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:DAC:RAWSET [deviceIndex] [dacChannel] [voltage]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ADC:GET [deviceIndex] [channel]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ADC:RAWGET [deviceIndex] [channel]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ADC:RESET [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ALLREGS:GET [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:OUTREGS:SET [deviceIndex] [val] [mask]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:INTERLOCK:DROP [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:INTERLOCK:RESTORE [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:FORCE:ON [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:FORCE:OFF [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:PERMISSION:ON [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:PERMISSION:OFF [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:STATUS:GET [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:DEVNAME:GET [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ADCNAME:GET [deviceIndex] [adcChannel]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:DACNAME:GET [deviceIndex] [dacChannel]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:INREGNAME:GET [deviceIndex] [inRegChannel]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:OUTREGNAME:GET [deviceIndex] [outRegChannel]</code></summary></details>
<details><summary><code>PSMCU:ALL:ADC:RESET</code></summary></details>
<details><summary><code>PSMCU:ALL:INTERLOCK:DROP</code></summary></details>
<details><summary><code>PSMCU:ALL:INTERLOCK:RESTORE</code></summary></details>
<details><summary><code>PSMCU:ALL:FORCE:ON</code></summary></details>
<details><summary><code>PSMCU:ALL:FORCE:OFF</code></summary></details>
<details><summary><code>PSMCU:ALL:PERMISSION:ON</code></summary></details>
<details><summary><code>PSMCU:ALL:PERMISSION:OFF</code></summary></details>

### Server data acquisition
<details><summary><code>PSMCU:CANGW:STATUS:GET</code></summary></details>
<details><summary><code>PSMCU:SERVER:NAME:GET</code></summary></details>
<details><summary><code>PSMCU:DEVNUM:GET</code></summary></details>
<details><summary><code>PSMCU:NAME2ID:GET</code></summary></details>

### Error status control

<details><summary><code>PSMCU:SINGLE:ERROR:SET [deviceIndex] [message]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ERROR:CLEAR [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:SINGLE:ERROR:GET [deviceIndex]</code></summary></details>
<details><summary><code>PSMCU:ALL:ERROR:SET [message]</code></summary></details>
<details><summary><code>PSMCU:ALL:ERROR:CLEAR</code></summary></details>
