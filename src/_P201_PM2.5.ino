#ifdef USES_P201
//#######################################################################################################
//#################################### Plugin 201  :GP2Y1051AU0F PM2.5 dust sensor ###########################
//#######################################################################################################
// The PMSx003 are particle sensors. Particles are measured by blowing air through the enclosure and,
// together with a laser, count the amount of particles. These sensors have an integrated microcontroller
// that counts particles and transmits measurement data over the serial connection.


#include <ESPeasySerial.h>
#include "_Plugin_Helper.h"

#define PLUGIN_201
#define PLUGIN_ID_201 201
#define PLUGIN_NAME_201 "Dust - PM2.5"
#define PLUGIN_VALUENAME1_201 "pm2.5"

#define PMSx003_SIG1 0XAA
#define PMSx003_SIZE 7

ESPeasySerial *P201_easySerial = nullptr;
boolean Plugin_201_init = false;
boolean values_received = false;

// Read 2 bytes from serial and make an uint16 of it. Additionally calculate
// checksum for PMSx003. Assumption is that there is data available, otherwise
// this function is blocking.
void SerialRead8(uint8_t* value, uint8_t* checksum)
{
  uint8_t data;

  // If P201_easySerial is initialized, we are using soft serial
  if (P201_easySerial == nullptr) return;
  data = P201_easySerial->read();

  *value = data;

  if (checksum != nullptr)
  {
    *checksum += data;
  }

#if 0
  // Low-level logging to see data from sensor
  String log = F("PM2.5 : byte=0x");
  log += String(data,HEX);
  log += F(" result=0x");
  log += String(*value,HEX);
  addLog(LOG_LEVEL_INFO, log);
#endif
}

void SerialFlush() {
  if (P201_easySerial != nullptr) {
    P201_easySerial->flush();
  }
}

boolean PacketAvailable(void)
{
  if (P201_easySerial != nullptr) // Software serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!P201_easySerial->available()) return false;
    while ((P201_easySerial->peek() != PMSx003_SIG1) && P201_easySerial->available()) {
      P201_easySerial->read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (P201_easySerial->available() < PMSx003_SIZE) return false; // Not enough yet for a complete packet
  }
  return true;
}

boolean Plugin_201_process_data(struct EventStruct *event) {
  uint8_t checksum = 0;
  uint8_t checksum2 = 0;
  uint8_t packet_header = 0;

  packet_header = P201_easySerial->read();
  if (packet_header != PMSx003_SIG1) {
    // Not the start of the packet, stop reading.
    return false;
  }

  uint8_t data[4]; // byte data_low, data_high;
  for (int i = 0; i < 4; i++)
    SerialRead8(&data[i], &checksum);

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("Vout(H)=");
    log += data[0];
    log += F(", Vout(L)=");
    log += data[1];
    log += F(", Vref(H)=");
    log += data[2];
    log += F(", Vref(L)=");
    log += data[3];
    addLog(LOG_LEVEL_DEBUG, log);
  }

  float vo = (data[0]*256.0 + data[1])/1024.0*5.00;
  float c = vo*700;

  // Compare checksums
  checksum2 = P201_easySerial->read();
  SerialFlush(); // Make sure no data is lost due to full buffer.
  if (checksum == checksum2)
  {
    // Data is checked and good, fill in output
    UserVar[event->BaseVarIndex]     = c;
    values_received = true;
    return true;
  }
  return false;
}

boolean Plugin_201(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_201;
        Device[deviceCount].Type = DEVICE_TYPE_SERIAL;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_201);
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_201));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += serialHelper_getSerialTypeLabel(event);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD: {
      serialHelper_webformLoad(event);
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      serialHelper_webformSave(event);
      success = true;
      break;
    }

    case PLUGIN_INIT:
      {
        int rxPin = CONFIG_PIN1;
        int txPin = CONFIG_PIN2;

        String log = F("PM2.5 : config ");
        log += rxPin;
        log += txPin;
        addLog(LOG_LEVEL_DEBUG, log);

        if (P201_easySerial != nullptr) {
          // Regardless the set pins, the software serial must be deleted.
          delete P201_easySerial;
          P201_easySerial = nullptr;
        }

        // Hardware serial is RX on 3 and TX on 1
        if (rxPin == 13 && txPin == 15)
        {
          log = F("PM2.53 : using hardware serial");
          addLog(LOG_LEVEL_INFO, log);
        }
        else
        {
          log = F("PMSx003: using software serial");
          addLog(LOG_LEVEL_INFO, log);
        }
        P201_easySerial = new ESPeasySerial(rxPin, txPin, false, 96); // 96 Bytes buffer, enough for up to 3 packets.
        P201_easySerial->begin(2400);
        P201_easySerial->flush();

        Plugin_201_init = true;
        success = true;
        break;
      }

    case PLUGIN_EXIT:
      {
          if (P201_easySerial)
          {
            delete P201_easySerial;
            P201_easySerial=nullptr;
          }
          break;
      }

    // The update rate from the module is 200ms .. multiple seconds. Practise
    // shows that we need to read the buffer many times per seconds to stay in
    // sync.
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_201_init)
        {
          // Check if a complete packet is available in the UART FIFO.
          if (PacketAvailable())
          {
            addLog(LOG_LEVEL_DEBUG_MORE, F("PM2.5 : Packet available"));
            success = Plugin_201_process_data(event);
          }
        }
        break;
      }
    case PLUGIN_READ:
      {
        // When new data is available, return true
        success = values_received;
        values_received = false;
        break;
      }
  }
  return success;
}
#endif // USES_P201
