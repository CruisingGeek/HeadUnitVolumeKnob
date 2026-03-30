// --------------------------------------------------------------------------------------------------------------------
// 
// Head Unit Volume Knob
// Scott DeBoer aka Cruising Geek
// 
// This project is used to interface a rotary encoder with aftermarket head units which have an SWC or remote control
// input. Specifically, this will be an analog input with three wires, normally a 1/8" stereo jack on the head unit.
// 
// By wiring the arduino and interfacing with the head unit, twisting the volume knob will allow the volume to be
// adjusted with the knob as opposed to using the buttons that most head units provide.
//
// You need to ensure that you have the analog steering wheel control (SWC) functionality that allows configuring
// the resistor values for the buttons. The two-wire digital steering wheel control will not work.
//
// Update 2025-Oct-19
// Because most aftermarket head units continue to provide 5v to the USB even if the keyed ignition is turned off,
// it is necessary to run the Arduino Pro Micro with an external 5v buck module to prevent buffer overflow in the
// code, and to prevent battery drain in the car. Due to this, I've also added support for Arduino UNO boards, which
// already have a 5v buck module built-on. If you have the physical space using that board provides a bit cleaner
// wiring.
// 
// --------------------------------------------------------------------------------------------------------------------

#include "Arduino.h"

#include "constants.h"
#include "HeadUnitDriver.h"

// --------------------------------------------------------------------------------------------------------------------
// LIBRARIES
// 
// These library includes are libraries you need to download and then copy into your /arduino/Libraries directory.
// --------------------------------------------------------------------------------------------------------------------

// Library location: https://github.com/mathertel/RotaryEncoder
#include <RotaryEncoder.h>
// Library location: https://github.com/CruisingGeek/CruisingGeekCommon
#include <Stopwatch.h>
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Variables
// --------------------------------------------------------------------------------------------------------------------

//
// To ensure the class memory is allocated and will be counted towards the total used variables in Arduino, directly
// construct the appropriate class instance here. Ensuring they have the same name prevents another #ifdef statement
// in the setup routine.
//
// In the future, if it is desired to support both via hardware (aka a DIP switch, jumper), just instantiate both and
// modify setup to select the correct one.
//
#if defined(USE_DIGITAL_HEADUNIT)
DigitalHeadUnitDriver driverInstance(MOSFET_DIGITAL_PIN, DIGITAL_COMMAND_PAUSE_MS, KenwoodCommand::Mute);
#else
AnalogHeadUnitDriver driverInstance(MOSFET_INCREASING_PIN, MOSFET_DECREASING_PIN, TEST_PIN);
#endif

// Encoder Variable; this does the heavy lifting to read the encoder values and update positions.
RotaryEncoder volumeKnob(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::FOUR3);

// HeadUnitDriver. The specific instance is currently set by firmware modification; in the future the board could have
// a dip switch or jumper to set this by the customer without firmware loading.

IHeadUnitDriver *headUnitDriver;

// Stopwatch variable to check the encoder for twists.
Stopwatch encoderStopwatch;

// Keeps track of the encoder's virtual position, which is the number of clicks forward minus number of clicks
// backwards since startup.
long previousPosition;
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Arduino Methods
// --------------------------------------------------------------------------------------------------------------------
void setup()
{
    #ifdef SERIAL_DEBUG
    {
        Serial.begin(9600);
    }
    #endif // SERIAL_DEBUG

    headUnitDriver = &driverInstance;

    pinMode(MOSFET_DECREASING_PIN, OUTPUT);
    pinMode(MOSFET_INCREASING_PIN, OUTPUT);
    pinMode(TEST_PIN, INPUT_PULLUP);

    digitalWrite(MOSFET_DECREASING_PIN, LOW);
    digitalWrite(MOSFET_INCREASING_PIN, LOW);

    encoderStopwatch.Start();
    headUnitDriver->StartDriver();
}

void loop()
{
    volumeKnob.tick();
    encoderStopwatch.Update();

    headUnitDriver->UpdateCounters();

    // if (encoderStopwatch.HasElapsed(ENCODER_READ_MS))
    {
        CheckForRotaryChange();
        encoderStopwatch.Reset();
    }

    headUnitDriver->RunIteration();
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Private methods
// --------------------------------------------------------------------------------------------------------------------
void CheckForRotaryChange()
{
    int newPos = volumeKnob.getPosition();
    int delta = abs(newPos - previousPosition);

    if (newPos > previousPosition)
    {
        headUnitDriver->HandleKnobChange(delta, InternalState::Decreasing);
    }
    else if (newPos < previousPosition)
    {
        headUnitDriver->HandleKnobChange(delta, InternalState::Increasing);
    }

    previousPosition = newPos;
}
// --------------------------------------------------------------------------------------------------------------------
