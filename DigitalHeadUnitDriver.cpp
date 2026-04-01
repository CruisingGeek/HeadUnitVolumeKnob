#include "Arduino.h"
#include "HeadUnitDriver.h"
#include "PulseGenerator.h"

#include <Stopwatch.h>

// --------------------------------------------------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------------------------------------------------

// Time of one tick of the communication in microseconds (uS).
const uint32_t TickTimeMicroSeconds = 562;

// This is the address that will be sent to the head unit that the kenwood style radios understand.
const uint8_t KenwoodAddress = 0xB9;
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------
DigitalHeadUnitDriver::DigitalHeadUnitDriver(
    uint8_t digitalMosfetPin,
    uint32_t commandPauseMS,
    KenwoodCommand buttonPressCommand)
{
    _mosfetPin = digitalMosfetPin;
    _commandPauseMS = commandPauseMS;

    _forceRun = false;
    _runKnobPressed = false;
    _count = 0;
    _state = InternalState::Off;
    _knobPressedCommand = buttonPressCommand;

    PulseGenerator::Instance().AssignPin(_mosfetPin);
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// IHeadUnitDriver interface methods
// --------------------------------------------------------------------------------------------------------------------
void DigitalHeadUnitDriver::StartDriver()
{
}

void DigitalHeadUnitDriver::UpdateCounters()
{
}

void DigitalHeadUnitDriver::RunIteration()
{
    if (_runKnobPressed
        && !PulseGenerator::Instance().IsSendInProgress())
    {
        WriteData(KenwoodAddress, _knobPressedCommand);
        _runKnobPressed = false;

        // Here we exit as we don't want to run an increment/decrement command right in a row after running the press
        // command. The increment/decrement will be caught on the next iteration.
        return;
    }

    if (
        _count != 0
        && !PulseGenerator::Instance().IsSendInProgress())
    {
        _forceRun = false;

        // Run one command per iteration; note that
        KenwoodCommand command = _state == InternalState::Increasing
            ? KenwoodCommand::VolumeUp
            : KenwoodCommand::VolumeDown;

        WriteData(KenwoodAddress, command);

        #ifdef SERIAL_DEBUG
        {
            String upDown = _state == InternalState::Decreasing
                ? "down"
                : _state == InternalState::Increasing
                    ? "up"
                    : "unknown";
            Serial.println("Sending volume command " + upDown + ".");
            Serial.print("Count: ");
            Serial.println(_count);
        }
        #endif
        int increment = _state == InternalState::Increasing ? -1 : 1;
        _count += increment;

        _state == _count == 0
            ? InternalState::Off
            : _count > 0
                ? InternalState::Increasing
                : InternalState::Decreasing;
    }
}

void DigitalHeadUnitDriver::HandleKnobChange(int delta, InternalState newState)
{
    if (_count == 0)
    {
        _state = newState;
        _forceRun = true;
    }

    _count += newState == InternalState::Increasing ? delta : -delta;
}

void DigitalHeadUnitDriver::HandleKnobPressed()
{
    _runKnobPressed = true;
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Write methods - idea taken from https://init6.pomorze.pl/projects/kenwood_ford/
// --------------------------------------------------------------------------------------------------------------------
void DigitalHeadUnitDriver::WriteData(uint8_t addr, KenwoodCommand command)
{
    PulseGenerator::Instance().SendPulseSequence(PulseFormat::NEC, addr, command);
}
// --------------------------------------------------------------------------------------------------------------------
