[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://github.com/finos2/Digital-DC-Load/graphs/commit-activity)
[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
# Electronic DC Load
A Arduino controled Dc load for PSU testing, battery characterization and more.

![DC load](/IMG/DSC001.jpg)

A usefull tool for testing Power supply stability and regulation , Battery capacity and discharge rate. 

## Specifications overview

* single channel, 40VDC /20 A, total power up to 800 W
* Adjustable current rising speed: _X_ A/μs to _Y_ A/μs
* Min. readback resolution: 0.1 mV，0.1 mA
* Overvoltage/overcurrent/overpower/overtemperature protection
* 4 static modes: CC, CV, CR, CP
* Built-in LXI/LAN communication interface


## Limitations
It cannot perfom any transient event simulation tests. The cuntrol algorythm utilized results in a low slew rate that does not allow any dynamic modes like pulsed, toggled or current spikes . 




# Legal
## License
The Hardware design files Of the Electronic DC load  are released under the [CERN-OHL-W Version 2.0](https://ohwr.org/cern_ohl_w_v2.txt) License.<br/>
All the source codes in this repository are released under the terms of the [GNU General Public License v3.0 License](https://github.com/finos2/Electronic-DC-Load/blob/main/LICENSE). 
### Disclamer 
 The datasheets included in this repository are for reference purposes only. All rights belong to their original creators, and we do not plan copyright infringement.