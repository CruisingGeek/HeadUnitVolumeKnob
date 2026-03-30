#ifndef __CruisingGeek_Constants_H__
#define __CruisingGeek_Constants_H__

// --------------------------------------------------------------------------------------------------------------------
// PINS
// --------------------------------------------------------------------------------------------------------------------
#if defined(ARDUINO_AVR_UNO)
    #define VOLUME_KNOB_SHIELD 1

    // Define these as the physical pins that the encoder is attached to.
    const uint8_t ENCODER_PIN_A = 8;
    const uint8_t ENCODER_PIN_B = 9;

    // Define these as the digital output pins in which the two N-channel MOSFETS connect.
    const uint8_t MOSFET_DECREASING_PIN = 12;
    const uint8_t MOSFET_INCREASING_PIN = 11;
    const uint8_t MOSFET_BUTTON_PIN = 10;

    //
    // Define the test pin. Setting this pin to GND will put the device in test mode which sends the signal for the
    // extended hold time, allowing you to configure the device via the android SwC app.
    //
    const uint8_t TEST_PIN = 13;

    //
    // Define the digital output pin that drives the MOSFET for the digital communication protocol, used in Kenwood
    // and select JVC models.
    //
    // For V2 pcbs, the MOSFET is shared but the user can select the correct resistor from the DIP switch.
    //
    // For V1.1 pcbs, there is only two available so we will reuse the increasing one from the analog output. This
    // means that the R1 resistor needs to be swapped though with one more in the 100 ohm range.
    //
    const uint8_t MOSFET_DIGITAL_PIN = 11;

    //
    // Knob button needs to be read by the CPU to handle the digital version, and its output sent to a transistor for
    // the analog version.
    //
    // For 1.x boards, this button is ONLY supported when using the digital headunit, aka Kenwood/JVC. That's because
    // there is no corresponding MOSFET to trigger the analog resistor. On 1.x boards when using the digital headunit
    // the wire for MUTE 1 needs to be moved directly to Pin 14, the PIN right above VIN on the rail with six inputs.
    //
    // For 2.x boards, just wire to the MUTE 1 and MUTE 2 pads as normal.
    //
    const uint8_t ENCODER_BUTTON_PIN = 14;
#else
    // Define these as the physical pins that the encoder is attached to.
    const uint8_t ENCODER_PIN_A = 2;
    const uint8_t ENCODER_PIN_B = 3;

    // Define these as the digital output pins in which the two N-channel MOSFETS connect.
    const uint8_t MOSFET_DECREASING_PIN = 8;
    const uint8_t MOSFET_INCREASING_PIN = 7;
    const uint8_t MOSFET_BUTTON_PIN = 9;

    // Define the test pin. Setting this pin to GND will put the device in test mode which
    // sends the signal for the extended hold time, allowing you to configure the device via
    // the android SwC app.
    const uint8_t TEST_PIN = 15;

    // Safe to reuse the normal volume MOSFET pin for the digital version.
    MOSFET_DIGITAL_PIN = 11;

    const uint8_t ENCODER_BUTTON_PIN = 5
#endif
// --------------------------------------------------------------------------------------------------------------------


// Uncomment this line to see debugging info
// #define SERIAL_DEBUG

// Uncomment this for the digital head unit
#define DIGITAL_HEADUNIT 1

// Set this to non-zero to have an initial start value for testing
#define INITIAL_START 0

// Define the pins for the IRremote library
#define IR_SEND_PIN 3

// Length of time in milliseconds (ms) to hold the mosfet open simulating a human touch for the analog driver.
//
// If this value is too short, the head unit will not register a simulated button press by an external user. If the
// value is too long, this program could miss increments if the person is spinning the knob rapidly, and the input lag
// becomes too noticeable.
//
const uint32_t MOSFET_HOLD_MS = 80;
const uint32_t MOSFET_HOLD_LOW_MS = 20;

// Length of time in milliseconds (ms) between sending digital commands for the digital driver.
//
// If this value is too long, the volume turn will have too much lag and feel sluggish. If the value is too small,
// tuns on the knob could be missed since the program will spend too many cycles on sending the command as opposed to
// reading the knob input.
//
const uint32_t DIGITAL_COMMAND_PAUSE_MS = 2;

// Length of time in milliseconds to wait before polling the encoder to see if the
// state changed.
const uint32_t ENCODER_READ_MS = 1;

// Length in time in milliseconds that MOSFET is held when in test mode.
const uint32_t MOSFET_HOLD_EXTENDED_MS = 4000;

#if defined(DIGITAL_HEADUNIT) and defined(VOLUME_KNOB_SHIELD)
#define USE_DIGITAL_HEADUNIT
#endif

#endif  // __CruisingGeek_Constants_H__
