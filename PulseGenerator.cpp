#include "Arduino.h"
#include "constants.h"
#include "PulseGenerator.h"

#include <TimerOne.h>

struct PulseFormatMetadata
{
    bool isLittleEndian;
    uint8_t startHigh;
    uint8_t startLow;
    uint8_t endHigh;
    uint8_t endLow;
    uint8_t tickHighZero;
    uint8_t tickLowZero;
    uint8_t tickHighOne;
    uint8_t tickLowOne;
    uint32_t pulseWidth;
};

/**
 * Map each entry of PulseFormat to a PulseMetadata instance with the appropriate data.
 * 
 * Ensure that the order is insync with the enum, as an array is being used for efficiency as opposed to an
 * unordered_map.
 */
const PulseFormatMetadata PulseDefinitions[] =
{
    // NEC
    {
        true,   // isLittleEndian
        16,     // startHigh
        8,      // startLow
        1,      // endHigh
        0,      // endLow
        1,      // tickHighZero
        1,      // tickLowZero
        1,      // tickHighOne
        3,      // tickLowOne
        562     // pulseWidth
    },
};

// Forward declare the helper method so it can be implemented at the bottom of the file.
uint32_t computeNextTargetCount(uint8_t index, uint8_t cycle, uint32_t data, PulseFormat format);

// Forward declare the reverseBits helper so it can be implemented at the bottom of the file.
uint8_t reverse8bit(uint8_t b);

/**
 * @brief   Wrapper class for PulseGenerator to allow SendCurrentPulse() to be a protected method.
 * 
 * @details In order to prevent end consumers of PulseGenerator from calling SendCurrentPulse, it needs to be
 *          a protected method. However, the interrupt handler can only call normal methods not class methods, and
 *          as such the SendCurrentPulse method would need to be public for it.
 * 
 *          To workaround this issue, the PulseGeneratorEx class wraps the PulseGenerator, and exposes the
 *          SendCurrentPulse() method. This allows it to be called from the interrupt handler, but also ensures the
 *          end consumer will not be able to call it directly.
 */
class PulseGeneratorEx : public PulseGenerator
{
public:
    PulseGeneratorEx() { };

    /**
     * @brief   Send the single current pulse cycle and set the timer for the next cycle to fire.
     * 
     * @details This method is defined here to allow it to be called from within HandleInterrupt, which is a
     *          method outside the class definition, and thus cannot access protected members.
     */
    void SendPulse()
    {
        SendCurrentPulse();
    }
};

/**
 * @brief   PulseGenerator instance. Will always be created even if never used.
 */
PulseGeneratorEx instance;

/**
 * @brief   Interrupt handler that is fired from the timer to generate the next pulse cycle.
 */
void HandleInterrupt()
{
    instance.SendPulse();
}

// --------------------------------------------------------------------------------------------------------------------
// Construction
// --------------------------------------------------------------------------------------------------------------------

/* static */
PulseGenerator& PulseGenerator::Instance()
{
    return instance;
}

/* private */
PulseGenerator::PulseGenerator()
{
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Public methods
// --------------------------------------------------------------------------------------------------------------------

void PulseGenerator::SendPulseSequence(PulseFormat format, uint32_t data)
{
    _isSending = true;

    _format = format;
    _data = data;

    _currentIndex = 0;
    _currentCycle = 0;
    _cycleCount = 0;

    // Setup the timer to fire at the appropriate time.
    uint32_t interruptMicros = PulseDefinitions[_format].pulseWidth;
    Timer1.stop();
    Timer1.initialize(interruptMicros);
    Timer1.attachInterrupt(HandleInterrupt);
    Timer1.restart();

    _isSending = true;
}

void PulseGenerator::SendPulseSequence(PulseFormat format, uint8_t addr, uint8_t command)
{
    uint8_t pAddr = addr;
    uint8_t nAddr = ~addr;
    uint8_t pCommand = command;
    uint8_t nCommand  = ~command;

    uint32_t data =
        ((uint32_t)nCommand << 24)
        | ((uint32_t)pCommand << 16)
        | ((uint32_t)nAddr << 8)
        | (uint32_t)pAddr;

    SendPulseSequence(format, data);
}

bool PulseGenerator::IsSendInProgress() const
{
    return _isSending;
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Protected Methods
// --------------------------------------------------------------------------------------------------------------------

void PulseGenerator::AssignPin(uint8_t pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void PulseGenerator::SendCurrentPulse()
{
    if (_currentIndex > 33)
    {
        // Done here, exit
        digitalWrite(_pin, LOW);
        Timer1.stop();
        _isSending = false;
        return;
    }

    _targetCount = computeNextTargetCount(_currentIndex, _currentCycle, _data, _format);

    // First write the value for this cycle.
    uint8_t valToWrite = _currentCycle == 0 ? HIGH : LOW;
    digitalWrite(_pin, valToWrite);

    // Increment the cycle count
    _cycleCount++;

    if (_cycleCount < _targetCount)
    {
        return;
    }

    // Move to the next cycle.
    _cycleCount = 0;
    _currentCycle++;

    if (_currentCycle > 1)
    {
        // Move to the first cycle of the next index
        _currentCycle = 0;
        _currentIndex++;
    }
}
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// Helper Methods
// --------------------------------------------------------------------------------------------------------------------

uint32_t computeNextTargetCount(uint8_t index, uint8_t cycle, uint32_t data, PulseFormat format)
{
    auto metadata = PulseDefinitions[format];

    if (index == 0)
    {
        // Start pulse
        return cycle == 0
            ? metadata.startHigh
            : metadata.startLow;
    }
    else if (index > 32)
    {
        // End pulse
        return cycle == 0
            ? metadata.endHigh
            : metadata.endLow;
    }

    //
    // Normal data case.
    //
    // Determine if the bit for the current index is zero or one. Then set the high/low bits accordingly.
    // 
    // This method sends data byte by byte from lowest byte to highest byte. Currently, for each byte it sends
    // using little endian; in the future, the value from the format table should be used to determine if it is
    // big endian, and do the shift accordingly.
    //
    uint32_t shiftedData = index == 1
        ? data
        : data >> (index - 1);
    uint32_t currentBit = shiftedData & 0x00000001;

    // Get the count of the next cycle based on the binary digit.
    return cycle == 0
        ? currentBit == 0
            ? metadata.tickHighZero
            : metadata.tickHighOne

        : currentBit == 0
            ? metadata.tickLowZero
            : metadata.tickLowOne;
}
// --------------------------------------------------------------------------------------------------------------------
