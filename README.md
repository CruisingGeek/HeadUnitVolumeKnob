# HeadUnitVolumeKnob
Putting this Arduino module between your aftermarket Android head unit with a rotary encoder allows volume control via the knob. Requires 2 n-channel MOSFETs to be driven from the Arduino Pro Micro

## Hardware
This code has been written to run on an Arduino Pro Micro. It should be easy to port it to other Arduino boards.

## Dependencies
This code depends on the CruisingGeekCommon library. You can find that library here: https://github.com/CruisingGeek/CruisingGeekCommon
Unpack that library into your Arduino libraries folder.

This code also depends on a 3rd party RotoryEncoder library you can find here: https://github.com/mathertel/RotaryEncoder
Unpack this into your libraries folder as well.

## MOSFET board
If you are building this project and would like the mosfet board to switch the two external resistors, please contact me @ CruisingGeek@gmail.com

## Wiring Diagrams
The following are wiring diagrams using both the MOSFET board and for generic MOSFETs

![Wiring diagram with TruDrive](/WiringDiagrams/VolumeKnobWiring-TruDrive.png)
![Wiring diagram with TruDrive](/WiringDiagrams/VolumeKnobWiring-MOSFETS.png)
