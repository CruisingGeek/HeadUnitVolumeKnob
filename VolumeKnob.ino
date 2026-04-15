// --------------------------------------------------------------------------------------------------------------------
// Head Unit Volume Knob
// Scott DeBoer aka Cruising Geek
// 
// This project is used to interface a rotary encoder with aftermarket head units which have an SWC or remote control
// input.
//
// Updated 2026-April-14
// This now supports all three of the major classes of head units on the market.
//  1. Android head units with configurable resistor values (ie the original Atoto this project was made for).
//  2. Analog head units that have Panosonic style hard-coded resistor values (namely Pioneer and Sony).
//  3. Head units with a single Remote input which communicate with an NEC digital protocol (Kenwood, JVC).
// 
// It is possible that there are more subtypes of case 2 that are unknown and would not be supported, but since most
// customers that are not using and Android-based one are using Pioneer or Kenwood this covers most cases.
//
// By wiring the arduino and interfacing with the head unit, twisting the volume knob will allow the volume to be
// adjusted with the knob as opposed to using the buttons that most head units provide. Further, a single press on the
// rotary encoder will mute the head unit (for some Pioneer models it will do attenuation, which is a super low
// volume but not fully muted). Of course with the Android configurable ones you can configure it to anything desired.
//
// Update 2025-Oct-19
// Because most aftermarket head units continue to provide 5v to the USB even if the keyed ignition is turned off,
// it is necessary to run the Arduino Pro Micro with an external 5v buck module to prevent buffer overflow in the
// code, and to prevent battery drain in the car. Due to this, I've also added support for Arduino UNO boards, which
// already have a 5v buck module built-on. If you have the physical space using that board provides a bit cleaner
// wiring.
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
// Library location: https://github.com/CruisingGeek/CruisingGeekCommon
#include <Button.h>
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
DigitalHeadUnitDriver driverInstance(
    MOSFET_DIGITAL_PIN,
    DIGITAL_COMMAND_PAUSE_MS,
    // Mute is the best option here, as play/pause and next track are context dependent in most Kenwood head units,
    // ie on the home screen do nothing, and in the radio do not what you'd expect. Mute on the other hand always
    // mutes.
    KenwoodCommand::Mute);
#else
AnalogHeadUnitDriver driverInstance(MOSFET_INCREASING_PIN, MOSFET_DECREASING_PIN, MOSFET_BUTTON_PIN, TEST_PIN);
#endif

// Encoder Variable; this does the heavy lifting to read the encoder values and update positions.
RotaryEncoder volumeKnob(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::FOUR3);

// Button attached to the rotary encoder.
Button knobButton(
    ENCODER_BUTTON_PIN,
    ButtonType::Momentary,
    ButtonPolarity::ActiveLow,
    1000, /* 1000ms */
    true /* usePullup */);

//
// HeadUnitDriver. The specific instance is currently set by firmware modification; in the future the board could have
// a dip switch or jumper to set this by the customer without firmware loading.
//
IHeadUnitDriver *headUnitDriver;

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
        Serial.begin(115200);
    }
    #endif // SERIAL_DEBUG

    headUnitDriver = &driverInstance;

    pinMode(MOSFET_DECREASING_PIN, OUTPUT);
    pinMode(MOSFET_INCREASING_PIN, OUTPUT);
    pinMode(MOSFET_BUTTON_PIN, OUTPUT);
    pinMode(TEST_PIN, INPUT_PULLUP);

    digitalWrite(MOSFET_DECREASING_PIN, LOW);
    digitalWrite(MOSFET_INCREASING_PIN, LOW);
    digitalWrite(MOSFET_BUTTON_PIN, LOW);

    headUnitDriver->StartDriver();
}

void loop()
{
    volumeKnob.tick();
    headUnitDriver->UpdateCounters();

    {
        // Rotary encoder input checks.
        CheckForRotaryChange();
        CheckForRotaryPress();
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

    if (delta == 0)
    {
        // Nothing to handle if the knob didn't change.
        return;
    }

    // NOTE - this compare has been swapped from V1.x so that the code matches reality (volume knob twist to right
    // will say increasing in the code)
    InternalState currentState = newPos < previousPosition
        ? InternalState::Decreasing
        : InternalState::Increasing;

    #if defined(SERIAL_DEBUG)
    {
        Serial.print("Handling ");
        Serial.println(currentState == InternalState::Increasing ? "Increasing" : "Decreasing");
    }
    #endif

    headUnitDriver->HandleKnobChange(delta, currentState);
    previousPosition = newPos;
}

void CheckForRotaryPress()
{
    ButtonState buttonState = knobButton.DetermineButtonState();

    if (buttonState == ButtonState::ShortPress)
    {
        headUnitDriver->HandleKnobPressed();
    }
}
// --------------------------------------------------------------------------------------------------------------------
