// DisAHRsterRadio: vereinheitlichte RadioLib-Layer1 (ersetzt Layer1_LoRa/Layer1_SX1276/Layer1_SX126x
// aus sudomesh/LoRaLayer2). Unterstuetzt SX1276 (RL_SX1276) und SX1262 (RL_SX1262), RadioLib >= 7.
#ifndef LAYER1_RADIOLIB_H
#define LAYER1_RADIOLIB_H
#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <packetBuffer.h>

#if defined(RL_SX1262)
typedef SX1262 L1Radio;
#elif defined(RL_SX1276)
typedef SX1276 L1Radio;
#else
#error "RL_SX1262 oder RL_SX1276 definieren"
#endif

class Layer1Class {
public:
    Layer1Class(L1Radio *lora, int mode, int cs, int reset, int dio, uint8_t sf = 9,
                float frequency = 869.525, int power = 14, float bandwidth = 125.0, float tcxo = 0.0);
    static int getTime() { return millis(); }
    int spreadingFactor() { return _spreadingFactor; }
    int loraInitialized() { return _loraInitialized; }
    int loraCSPin() { return _csPin; }
    int resetPin() { return _resetPin; }
    int DIOPin() { return _DIOPin; }
    long spiFrequency() { return _spiFrequency; }
    float loraFrequency() { return _loraFrequency; }
    int txPower() { return _txPower; }
    void setTxPower(int p);
    void setSpreadingFactor(uint8_t sf);
    packetBuffer *txBuffer;
    packetBuffer *rxBuffer;
    int init();
    int transmit();
    int receive();
private:
    static void setFlag(void);
    int sendPacket(char *data, size_t len);
    L1Radio *_LoRa;
    int _mode, _csPin, _resetPin, _DIOPin;
    uint8_t _spreadingFactor;
    float _loraFrequency;
    int _txPower;
    int _loraInitialized;
    long _spiFrequency;
    float _bandwidth;
    float _tcxo;
};
#endif
