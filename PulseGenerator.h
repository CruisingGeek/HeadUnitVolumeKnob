#ifndef __CruisingGeek_PulseGenerator__
#define __CruisingGeek_PulseGenerator__

// Start pair + 32 bit data pair + end pair = 34 pairs.
#define NUMBER_OF_PULSE_PAIRS 34

/**
 * Define the format to use, which selects the the start and end timing plus pulse length and shape.
 */
enum PulseFormat : uint8_t
{
    NEC,
};

class PulseGenerator
{
public:
    /**
     * @brief   Get the singleton PulseGenerator instance.
     */
    static PulseGenerator& Instance();

    /**
     * @brief   Assign the pin to use with this instance.
     * 
     * @param   pin
     *          The digital pin number associated with the pin where the pulses are generated.
     */
    void AssignPin(uint8_t pin);

    /**
     * @brief   Sends a pulse train for the provided data.
     * 
     * @details Using the provided format, a pulse train will be sent. 
     * 
     * @param   format
     *          Format of the pulse to send.
     * 
     * @param   data
     *          Raw data to be sent; data is sent with least significant to most significant 8 bit chunks, using little
     *          endian for each chunk.
     */
    void SendPulseSequence(PulseFormat format, uint32_t data);

    /**
     * @brief   Sends a pulse train for the provided address and command combo.
     * 
     * @details Using the provided format, a pulse train will be constructed and sent. This handles both sending the
     *          reversed address and command, as well as ensuring sending in the correct big or little endian.
     * 
     * @param   format
     *          Format of the pulse to send.
     * 
     * @param   address
     *          Destination address for the device receiving the pulse.
     * 
     * @param   command
     *          Desired command to be executed on the target device.
     */
    void SendPulseSequence(PulseFormat format, uint8_t addr, uint8_t command);

    /**
     * @brief   Indicates if there is an active send in progress.
     */
    bool IsSendInProgress() const;

protected:
    /**
     * @brief   Constructs an instance of the PulseGenerator class.
     * 
     * @details Constructs an instance of the PulseGenerator class for the provided parameters. Private constructor as
     *          this class uses a singleton pattern to wrap the timer function.
     */
    PulseGenerator();

    /**
     * @brief   Writes to the wire the current pulse and sets the timer to fire when its complete.
     * 
     * @details NOTE: This method is protected as it is handled internally by the timer and shouldn't be called
     *          directly from the consuming code.
     */
    void SendCurrentPulse();

    PulseFormat _format;
    uint32_t _data;
    uint8_t _pin;

    uint8_t _currentIndex;
    uint8_t _currentCycle;
    uint32_t _cycleCount;

    uint32_t _targetCount;

    bool _isSending;
};

#endif // __CruisingGeek_PulseGenerator__