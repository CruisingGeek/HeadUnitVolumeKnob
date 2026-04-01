#ifndef __CruisingGeek_HeadUnitDriver__
#define __CruisingGeek_HeadUnitDriver__

#include "Arduino.h"
#include "constants.h"

// Forward declare Stopwatch class
class Stopwatch;

/**
 * Defines the current state the driver can be in.
 */
enum InternalState : uint8_t
{
    Off,
    Decreasing,
    Increasing,
    HoldLow,
};

/**
 * List of possible commands that can be sent to the head unit.
 */
enum KenwoodCommand : uint8_t
{
    PreviousTrack = 0x0A,
    NextTrack = 0x0B,
    Reverse = 0x0C,
    FastForward = 0x0D,
    PlayPause = 0x0E,
    Source = 0x13,
    VolumeUp = 0x14,
    VolumeDown = 0x15,
    Mute = 0x16,
    Tuner = 0x1C,
    Tape = 0x1D,
    CD = 0x1E,
    CD_MD_CH = 0x1F,
    DNPP = 0x5E,
};

/**
 * Interface for a HeadUnitDriver instance. This is used to generically by the main program to abstract the
 * implementation details related to the head unit, as some head units are analog and some are digital.
 */
struct IHeadUnitDriver
{
    /**
     * Starts the instance associated with this driver. Should be called in the Arduino setup() method.
     */
    virtual void StartDriver() = 0;

    /**
     * Update all internal counters associated with this instance. Should be called from the Arduino loop() method,
     * once per loop.
     */
    virtual void UpdateCounters() = 0;

    /**
     * Run all timer associated actions for this driver. Should be called in the Arduino loop() method, once per loop.
     */
    virtual void RunIteration() = 0;

    /**
     * Call when a change has been detected in the volume knob with the new delta, and the new state.
     * 
     * @param delta
     *      Defines the difference from zero 
     */
    virtual void HandleKnobChange(int delta, InternalState newState) = 0;

    /**
     * Call when the button integrated in the knob was pressed.
     */
    virtual void HandleKnobPressed() = 0;
};
// --------------------------------------------------------------------------------------------------------------------


/**
 * Driver that is used for analog head units. This is most head units including the android-based configurable ones and
 * the hard-coded ones we directly support namely Pioneer and Sony.
 */
class AnalogHeadUnitDriver : public IHeadUnitDriver
{
public:
    AnalogHeadUnitDriver(uint8_t increasingPin, uint8_t decreasingPin, uint8_t testPin);

    // IHeadUnitDriver interface methods
    void StartDriver();
    void UpdateCounters();
    void RunIteration();
    void HandleKnobChange(int delta, InternalState newState);
    void HandleKnobPressed();

private:
    void TurnOffMosfets();
    void UpdateLengthToHold();

    InternalState _state;
    uint8_t _iPin, _dPin, _tPin;
    uint32_t _lengthToHold;
    long _count;
    Stopwatch *_mosfetStopwatch;
    Stopwatch *_holdLowStopwatch;
};

/**
 * Driver that is used for analog head units. This is most head units including the android-based configurable ones and
 * the hard-coded ones we directly support namely Pioneer and Sony.
 */
class DigitalHeadUnitDriver : public IHeadUnitDriver
{
public:
    /**
    * Construct a new instance of a digital head unit driver, with the optional parameter to specific which command is
    * sent when the knob is pressed.
    * 
    * @param    digitalMosfetPin
    *           Pin which drives the mosfet to ground the communication wire. Kenwood style digital interface has a
    *           line with pullup for high, so an N-channel mosfet to ground represents low.
    * 
    * @param    commandPauseMS
    *           Length in milliseconds (ms) between commands being sent. Set this to a value greater than 1 to ensure
    *           the line is not bombarded with back to back commands.
    * 
    * @param    buttonPressCommand
    *           Command that is sent when the knob is pressed. Default is KenwoodCommand::Mute.
    */
    DigitalHeadUnitDriver(
        uint8_t digitalMosfetPin,
        uint32_t commandPauseMS,
        KenwoodCommand buttonPressCommand = KenwoodCommand::Mute);

    // IHeadUnitDriver interface methods
    void StartDriver();
    void UpdateCounters();
    void RunIteration();
    void HandleKnobChange(int delta, InternalState newState);
    void HandleKnobPressed();

private:
    /**
    * Write the 32bit data to the wire.
    * 
    * @param    addr
    *           Address for the device that is being targeted
    * 
    * @param    command
    *           Hex value for the command to execute.
    */
    void WriteData (uint8_t addr, KenwoodCommand command);

    InternalState _state;
    uint8_t _mosfetPin;
    uint32_t _commandPauseMS;
    long _count;
    boolean _forceRun, _runKnobPressed;
    KenwoodCommand _knobPressedCommand;
};

#endif  // __CruisingGeek_HeadUnitDriver__
