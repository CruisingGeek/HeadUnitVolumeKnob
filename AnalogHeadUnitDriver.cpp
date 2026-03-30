#include "HeadUnitDriver.h"

#include <Stopwatch.h>

// --------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------
AnalogHeadUnitDriver::AnalogHeadUnitDriver(uint8_t increasingPin, uint8_t decreasingPin, uint8_t testPin)
{
    _iPin = increasingPin;
    _dPin = decreasingPin;
    _tPin = testPin;

    _state = InternalState::Off;
    _lengthToHold = MOSFET_HOLD_MS;
    _count = 0;

    _mosfetStopwatch = new Stopwatch();
    _holdLowStopwatch = new Stopwatch();
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// IHeadUnitDriver interface methods
// --------------------------------------------------------------------------------------------------------------------
void AnalogHeadUnitDriver::StartDriver()
{
    _mosfetStopwatch->Start();

    #if INITIAL_START != 0
    {
        count = 0;
        state = InternalState::Off;
    }
    #endif  // INITIAL_START != 0
}

void AnalogHeadUnitDriver::UpdateCounters()
{
    if (_state == InternalState::Decreasing || _state == InternalState::Increasing)
    {
        _mosfetStopwatch->Update();
    }
    else
    {
        _mosfetStopwatch->Reset();
    }

    if (_state == InternalState::HoldLow)
    {
        _holdLowStopwatch->Update();
    }
    else
    {
        _holdLowStopwatch->Reset();
    }
}

void AnalogHeadUnitDriver::RunIteration()
{
    if ((_state == InternalState::Increasing || _state == InternalState::Decreasing)
         && _mosfetStopwatch->HasElapsed(_lengthToHold))
    {
        _count += _state == InternalState::Increasing ? -1 : 1;
        _state = InternalState::HoldLow;

        TurnOffMosfets();
        UpdateLengthToHold();
        _mosfetStopwatch->Reset();
        
    }

    if (_state == InternalState::HoldLow && _holdLowStopwatch->HasElapsed(MOSFET_HOLD_LOW_MS))
    {
        _state = _count == 0
            ? InternalState::Off
            : _count > 0
                ? InternalState::Increasing
                : InternalState::Decreasing;
        
        if (_state != InternalState::Off)
        {
            uint8_t pin = _state == InternalState::Decreasing ? _dPin : _iPin;
            digitalWrite(pin, HIGH);
        }
    }
}

void AnalogHeadUnitDriver::HandleKnobChange(int delta, InternalState newState)
{
    #ifdef SERIAL_DEBUG
    {
        Serial.print("Handling ");
        Serial.println(newState == InternalState::Decreasing ? "Decreasing" : "Increasing");
    }
    #endif // SERIAL_DEBUG

    if (_count == 0)
    {
        uint8_t pin = newState == InternalState::Decreasing ? _dPin : _iPin;
        digitalWrite(pin, HIGH);

        // TODO test this.
        _state = newState;
    }

    _count += _state == InternalState::Increasing ? delta : -delta;
}

void AnalogHeadUnitDriver::HandleKnobPressed()
{
    // V1.1 of the board there's nothing to do here.
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Private methods
// --------------------------------------------------------------------------------------------------------------------
void AnalogHeadUnitDriver::TurnOffMosfets()
{
    #ifdef SERIAL_DEBUG
    {
        Serial.println("Turning off mosfets");
    }
    #endif // SERIAL_DEBUG

    digitalWrite(_dPin, LOW);
    digitalWrite(_iPin, LOW);
}

void AnalogHeadUnitDriver::UpdateLengthToHold()
{
    uint32_t testPinVal = digitalRead(_tPin);
    _lengthToHold = testPinVal == LOW ? MOSFET_HOLD_EXTENDED_MS : MOSFET_HOLD_MS;
}
// --------------------------------------------------------------------------------------------------------------------
