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
#define PLUGIN_VALUENAME1_201 "PM2.5"

ESPeasySerial *P201_easySerial;
boolean Plugin_201_init = false;

int incomeByte[7];
int data;
int z = 0;
int sum;

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

        P201_easySerial = new ESPeasySerial(rxPin, txPin, false, 0); // 96 Bytes buffer, enough for up to 3 packets.
        P201_easySerial->begin(2400);
        P201_easySerial->flush();

        Plugin_201_init = true;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        while (P201_easySerial->available() > 0) {
         data = P201_easySerial->read();
          if (data == 170) {
             z = 0;
             incomeByte[z] = data;
          }
          else {
               z++;
               incomeByte[z] = data;
          }
          if (z == 6)
          {
           sum = incomeByte[1] + incomeByte[2] + incomeByte[3] + incomeByte[4];
            if (incomeByte[5] == sum && incomeByte[6] == 255 )
            {
               float vo = (incomeByte[1] * 256.0 + incomeByte[2]) / 1024.0 * 5.00;
               float c = vo * 700;
               UserVar[event->BaseVarIndex] = (float)c;
               break;
            }
            else {
               z = 0;
                P201_easySerial->flush();
                data = 0;
                for (int m = 0; m < 7; m++) {
                   incomeByte[m] = 0;
                   break;
                }
            }
            z=0;
            break;
          }
        }
        success = true;
        break;
      }
  }
  return success;
}
#endif // USES_P201
