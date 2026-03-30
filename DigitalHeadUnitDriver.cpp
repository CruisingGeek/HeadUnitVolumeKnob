#include "Arduino.h"
#include "HeadUnitDriver.h"

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

    _commandStopwatch = new Stopwatch();
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// IHeadUnitDriver interface methods
// --------------------------------------------------------------------------------------------------------------------
void DigitalHeadUnitDriver::StartDriver()
{
    _commandStopwatch->Start();
}

void DigitalHeadUnitDriver::UpdateCounters()
{
    if (_count != 0)
    {
        _commandStopwatch->Update();
    }
    else
    {
        _commandStopwatch->Reset();
    }
}

void DigitalHeadUnitDriver::RunIteration()
{
    if (_runKnobPressed)
    {
        WriteData(KenwoodAddress, _knobPressedCommand);
        _runKnobPressed = false;

        // Here we exit as we don't want to run an increment/decrement command right in a row after running the press
        // command. The increment/decrement will be caught on the next iteration.
        return;
    }

    if (
        _count != 0
        && (_forceRun || _commandStopwatch->HasElapsed(_commandPauseMS)))
    {
        _commandStopwatch->Reset();
        _forceRun = false;

        // Run one command per iteration; note that
        KenwoodCommand command = _state == InternalState::Increasing
            ? KenwoodCommand::VolumeUp
            : KenwoodCommand::VolumeDown;

        WriteData(KenwoodAddress, command);

        int increment = _state == InternalState::Increasing ? -1 : 1;
        _count += increment;

        _state == _count == 0
            ? InternalState::Off
            : _count > 0
                ? InternalState::Increasing
                : InternalState::Decreasing;

        #if defined(SERIAL_DEBUG)
        {
            Serial.print("CurrentState: ");
            Serial.println(_state);
            Serial.print("Count: ");
            Serial.println(_count);
        }
        #endif
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
    uint8_t naddr = ~addr;

    uint8_t data = command;
    uint8_t ndata  = ~data;

    WriteStart();
    Write8bit (&addr);
    Write8bit (&naddr);
    Write8bit (&data);
    Write8bit (&ndata);
    WriteFinish();
}

void DigitalHeadUnitDriver::WriteBase(uint8_t holdCountHighMicroS, uint8_t holdCountLowMicroS)
{
    uint8_t hm = holdCountHighMicroS;
    uint8_t lm = holdCountLowMicroS;

    digitalWrite(_mosfetPin, HIGH);
    for (uint8_t i = 0; i < hm; i++)
    {
        delayMicroseconds(TickTimeMicroSeconds);
    }

    digitalWrite(_mosfetPin, LOW);
    for (uint8_t i = 0; i < lm; i++)
    {
        delayMicroseconds(TickTimeMicroSeconds);
    }
}

void DigitalHeadUnitDriver::Write8bit(uint8_t *bits)
{
    for (uint8_t i=0; i<8; i++)
    {
        if (*bits & 0x01 == 0x01)
        {
            WriteBinaryOne();
        }
        else
        {
            WriteBinaryZero();
        }
        *bits = *bits >> 1;
    }
}

void DigitalHeadUnitDriver::WriteStart(void)
{
    // 16 * 562us = 9ms high, 8 * 562us = 4.5ms low
    WriteBase(16, 8);
}

void DigitalHeadUnitDriver::WriteFinish(void)
{
    // To finish only write an extra high pulse.
    WriteBase(1, 0);
}

void DigitalHeadUnitDriver::WriteBinaryOne(void)
{
    // 562.5us high, 562us * 3 = 1.6875us low.
    WriteBase(1, 3);
}

void DigitalHeadUnitDriver::WriteBinaryZero(void)
{
    // 562.5us high, 562.5us low
    WriteBase(1, 1);
}
// --------------------------------------------------------------------------------------------------------------------
