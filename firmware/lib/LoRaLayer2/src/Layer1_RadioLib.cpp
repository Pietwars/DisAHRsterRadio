#include <Layer1_RadioLib.h>

static volatile bool _dioFlag = false;
static volatile bool _transmitFlag = false;
uint32_t Layer1Class::_holdUntil = 0;

Layer1Class::Layer1Class(L1Radio *lora, int mode, int cs, int reset, int dio, uint8_t sf,
                         float frequency, int power, float bandwidth, float tcxo)
: _LoRa(lora), _mode(mode), _csPin(cs), _resetPin(reset), _DIOPin(dio), _spreadingFactor(sf),
  _loraFrequency(frequency), _txPower(power), _loraInitialized(0), _spiFrequency(2000000),
  _bandwidth(bandwidth), _tcxo(tcxo)
{
    txBuffer = new packetBuffer();
    rxBuffer = new packetBuffer();
}

#if defined(ESP32)
IRAM_ATTR
#endif
void Layer1Class::setFlag(void) { _dioFlag = true; }

int Layer1Class::init() {
#if defined(RL_SX1262)
    int state = _LoRa->begin(_loraFrequency, _bandwidth, _spreadingFactor, 5,
                             RADIOLIB_SX126X_SYNC_WORD_PRIVATE, _txPower, 8, _tcxo, false);
    if (state == RADIOLIB_ERR_NONE) {
        _LoRa->setDio2AsRfSwitch(true);
        _LoRa->setCurrentLimit(140);
    }
#else
    int state = _LoRa->begin(_loraFrequency, _bandwidth, _spreadingFactor, 5,
                             RADIOLIB_SX127X_SYNC_WORD, _txPower, 8, 0);
#endif
    Serial.printf("Layer1::init(): RadioLib state = %d\r\n", state);
    if (state != RADIOLIB_ERR_NONE) return 0;
    _LoRa->setPacketReceivedAction(Layer1Class::setFlag);
    if (_LoRa->startReceive() != RADIOLIB_ERR_NONE) return 0;
    _loraInitialized = 1;
    return 1;
}

void Layer1Class::setTxPower(int p) {
    if (_LoRa->setOutputPower(p) == RADIOLIB_ERR_NONE) _txPower = p;
}

void Layer1Class::setSpreadingFactor(uint8_t sf) {
    if (_LoRa->setSpreadingFactor(sf) == RADIOLIB_ERR_NONE) _spreadingFactor = sf;
    _LoRa->startReceive();
}

int Layer1Class::sendPacket(char *data, size_t len) {
    _transmitFlag = true;
    int state = _LoRa->startTransmit((uint8_t *)data, len);
    if (state == RADIOLIB_ERR_NONE) return 0;
    _transmitFlag = false;
    _LoRa->startReceive();
    if (state == RADIOLIB_ERR_PACKET_TOO_LONG) return 1;
    if (state == RADIOLIB_ERR_TX_TIMEOUT) return 2;
    return 3;
}

int Layer1Class::transmit() {
    if (_transmitFlag) return 0; // laufende Aussendung abwarten
    if ((int32_t)(millis() - _holdUntil) < 0) return 0; // Sendepause (WLAN-Start)
    BufferEntry entry = txBuffer->read();
    if (entry.length > 0) sendPacket(entry.data, entry.length);
    return entry.length;
}

int Layer1Class::receive() {
    if (!_dioFlag) return 0;
    _dioFlag = false;
    if (_transmitFlag) {
        _transmitFlag = false;
        _LoRa->finishTransmit();
        _LoRa->startReceive();
        return 0;
    }
    int ret;
    size_t len = _LoRa->getPacketLength();
    if (len == 0 || len > sizeof(((BufferEntry *)0)->data)) { _LoRa->startReceive(); return -2; }
    uint8_t data[256];
    int state = _LoRa->readData(data, len);
    if (state == RADIOLIB_ERR_NONE) {
        BufferEntry entry;
        memcpy(&entry.data[0], data, len);
        entry.length = len;
        rxBuffer->write(entry);
        ret = len;
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) ret = -1;
    else ret = -2;
    _LoRa->startReceive();
    return ret;
}
