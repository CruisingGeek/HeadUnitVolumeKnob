#include "Arduino.h"
#include <Encoder.h>
#include <Stopwatch.h>

// ------------------------------------------------------------------------------------
// CONSTANTS
// ------------------------------------------------------------------------------------
// Uncomment this line to see debugging info
// #define SERIAL_DEBUG

// Define these as the physical pins that the encoder is attached to.
const uint8_t ENCODER_PIN_A = 7;
const uint8_t ENCODER_PIN_B = 8;

// Define these as the digital output pins in which the two N-channel MOSFETS connect.
const uint8_t MOSFET_DECREASING = 3;
const uint8_t MOSFET_INCREASING = 2;

// Define the test pin. Setting this pin to GND will put the device in test mode which
// sends the signal for the extended hold time, allowing you to configure the device via
// the android SwC app.
const uint8_t TEST_PIN = 15;

// Length of time in milliseconds to hold the moseft open simulating a human touch.
//
// If this value is too short, the head unit will not register a simulated button press
// by an external user. If the value is too long, this program could miss increments if
// the person is spinning the knob rapidly.
const uint32_t MOSFET_HOLD_MS = 100;
const uint32_t MOSFET_HOLD_LOW_MS = 4;

// Length of time in milliseconds to wait before polling the encoder to see if the
// state changed.
const uint32_t ENCODER_READ_MS = 1;

// Length in time in milliseconds that MOSFET is held when in test mode.
const uint32_t MOSFET_HOLD_EXTENDED_MS = 4000;
// ------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------
// Enums
// ------------------------------------------------------------------------------------
enum InternalState : uint8_t
{
    Off,
    Decreasing,
    Increasing,
    HoldLow,
};
// ------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------
// Variables
// ------------------------------------------------------------------------------------
// Encoder Variable; this does the heavy lifting to read the encoder values and update positions.
Encoder volumeKnob(ENCODER_PIN_A, ENCODER_PIN_B);

// Stopwatch variables to trigger turning off the MOSFETs and checking the encoder
Stopwatch encoderStopwatch;
Stopwatch mosfetStopwatch;
Stopwatch holdLowStopwatch;

// This stores the previous encoder value to see if the value has changed.
long previousEncoderValue = 0;

// Keeps track of when the MOSFETS need to be shut off.
InternalState state = InternalState::Off;

uint32_t lengthToHold;

long count = 0;
// ------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------
// Arduino Methods
// ------------------------------------------------------------------------------------
void setup()
{
    #ifdef SERIAL_DEBUG
    {
        Serial.begin(9600);
    }
    #endif // SERIAL_DEBUG

    pinMode(MOSFET_DECREASING, OUTPUT);
    pinMode(MOSFET_INCREASING, OUTPUT);
    pinMode(TEST_PIN, INPUT_PULLUP);
    digitalWrite(MOSFET_DECREASING, LOW);
    digitalWrite(MOSFET_INCREASING, LOW);

    encoderStopwatch.Start();
    mosfetStopwatch.Start();

    lengthToHold = MOSFET_HOLD_MS;
}

void loop()
{
    encoderStopwatch.Update();

    if (state == InternalState::Decreasing || state == InternalState::Increasing)
    {
        mosfetStopwatch.Update();
    }
    else
    {
        mosfetStopwatch.Reset();
    }

    if (state == InternalState::HoldLow)
    {
        holdLowStopwatch.Update();
    }
    else
    {
        holdLowStopwatch.Reset();
    }

    //if (encoderStopwatch.HasElapsed(ENCODER_READ_MS))
    {
        CheckForRotaryChange();
        encoderStopwatch.Reset();
    }

    if ((state == InternalState::Increasing || state == InternalState::Decreasing)
         && mosfetStopwatch.HasElapsed(lengthToHold))
    {
        HandleTurnOffMosfets();
        mosfetStopwatch.Reset();
        uint32_t testPinVal = digitalRead(TEST_PIN);
        lengthToHold = testPinVal == LOW ? MOSFET_HOLD_EXTENDED_MS : MOSFET_HOLD_MS;
    }

    if (state == InternalState::HoldLow && holdLowStopwatch.HasElapsed(MOSFET_HOLD_LOW_MS))
    {
        state = count == 0
            ? InternalState::Off
            : count > 0
                ? InternalState::Increasing
                : InternalState::Decreasing;
        
        if (state != InternalState::Off)
        {
            uint8_t pin = state == InternalState::Decreasing ? MOSFET_DECREASING : MOSFET_INCREASING;
            digitalWrite(pin, HIGH);
        }
    }
}
// ------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------
// Private methods
// ------------------------------------------------------------------------------------
void CheckForRotaryChange()
{
    long currentValue = volumeKnob.read();

    if (currentValue > previousEncoderValue && state != InternalState::Decreasing)
    {
        HandleKnobChange(InternalState::Decreasing);
    }
    else if (currentValue < previousEncoderValue && state != InternalState::Increasing)
    {
        HandleKnobChange(InternalState::Increasing);
    }

    previousEncoderValue = currentValue;
}

void HandleKnobChange(InternalState newState)
{
    #ifdef SERIAL_DEBUG
    {
        Serial.print("Handling ");
        Serial.println(newState == InternalState::Decreasing ? "Decreasing" : "Increasing");
    }
    #endif // SERIAL_DEBUG

    if (count == 0)
    {
        uint8_t pin = newState == InternalState::Decreasing ? MOSFET_DECREASING : MOSFET_INCREASING;
        digitalWrite(pin, HIGH);
    }
    count += newState == InternalState::Decreasing ? -1 : 1;
    state = newState;
}

void HandleTurnOffMosfets()
{
    #ifdef SERIAL_DEBUG
    {
        Serial.println("Turning off mosfets");
    }
    #endif // SERIAL_DEBUG

    count += state == InternalState::Increasing ? -1 : 1;
    state = InternalState::HoldLow;
    digitalWrite(MOSFET_DECREASING, LOW);
    digitalWrite(MOSFET_INCREASING, LOW);
}
// ------------------------------------------------------------------------------------
