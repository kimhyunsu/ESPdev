#ifdef USES_P200
//#######################################################################################################
//#################################### Plugin 012: LCD ##################################################
//#######################################################################################################

#include "_Plugin_Helper.h"

// Sample templates
//  Temp: [DHT11#Temperature]   Hum:[DHT11#humidity]
//  DS Temp:[Dallas1#Temperature#R]
//  Lux:[Lux#Lux#R]
//  Baro:[Baro#Pressure#R]
//  Pump:[Pump#on#O] -> ON/OFF
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library


#define PLUGIN_200
#define PLUGIN_ID_200         200
#define PLUGIN_NAME_200       "Display - TTGO"
#define PLUGIN_VALUENAME1_200 "TFTLCD"

#define TFT_DISPOFF 0x28
#define TFT_SLPIN   0x10

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23
#define TFT_BL              14   // Display backlight control pin

int Plugin_200_cols = 16;
int Plugin_200_rows = 2;

#define P200_Nlines 2        // The number of different lines which can be displayed
#define P200_Nchars 20


boolean Plugin_200(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte displayTimer = 0;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_200;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_NONE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_200);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_200));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice1 = PCONFIG(0);
        String options1[2];
        options1[0] = F("0");
        options1[1] = F("1");
        int optionValues1[2] = { 0, 1 };
        addFormSelector(F("Rotat"), F("p200_rotat"), 2, options1, optionValues1, choice1);

        byte choice2 = PCONFIG(1);
        String options2[9];
        options2[0] = F("Top left");
        options2[1] = F("Top centre");
        options2[2] = F("Top right");
        options2[3] = F("Middle left");
        options2[4] = F("Middle centre");
        options2[5] = F("Middle right");
        options2[6] = F("Bottom left");
        options2[7] = F("Bottom centre");
        options2[8] = F("Bottom right");
        int optionValues2[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        addFormSelector(F("Datum"), F("p200_datun"), 9, options2, optionValues2, choice2);

        byte choice3 = PCONFIG(2);
        String options3[7];
        options3[0] = F("glcdfont");
        options3[1] = F("Font7srle");
        options3[2] = F("Font16");
        options3[3] = F("Font32rle");
        options3[4] = F("Font64rle");
        options3[5] = F("Font72rle");
        options3[6] = F("Font72x53rle");
        int optionValues3[7] = { 1, 2, 3, 4, 5, 6, 7 };
        addFormSelector(F("Font"), F("p200_font"), 7, options3, optionValues3, choice3);

        {
          String strings[P200_Nlines];
          LoadCustomTaskSettings(event->TaskIndex, strings, P200_Nlines, P200_Nchars);
          for (byte varNr = 0; varNr < 2; varNr++)
          {
            addFormTextBox(String(F("Line ")) + (varNr + 1), getPluginCustomArgName(varNr), strings[varNr], 30);
          }
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p200_rotat"));
        PCONFIG(1) = getFormItemInt(F("p200_datun"));
        PCONFIG(2) = getFormItemInt(F("p200_font"));

        // FIXME TD-er: This is a huge stack allocated object.
        char deviceTemplate[P200_Nlines][P200_Nchars];
        String error;
        for (byte varNr = 0; varNr < P200_Nlines; varNr++)
        {
          if (!safe_strncpy(deviceTemplate[varNr], web_server.arg(getPluginCustomArgName(varNr)), P200_Nchars)) {
            error += getCustomTaskSettingsError(varNr);
          }
        }
        if (error.length() > 0) {
          addHtmlError(error);
        }
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        tft.init();
        tft.setRotation(PCONFIG(0));
        tft.fillScreen(TFT_WHITE);
        tft.setTextSize(PCONFIG(2));
        tft.setTextColor(TFT_BLACK);
        //tft.setCursor(20, 30);
        tft.setTextDatum(PCONFIG(1));
      //  tft.drawString("TEMP",10,50);
      //  tft.drawString("HUM",10,100);

        if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        }
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        // FIXME TD-er: This is a huge stack allocated object.
        char deviceTemplate[P200_Nlines][P200_Nchars];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        addLog(LOG_LEVEL_DEBUG_MORE, F("PM2.5 : data read"));
        addLog(LOG_LEVEL_DEBUG_MORE, deviceTemplate[0]);

      for (byte x = 0; x < Plugin_200_rows; x++)
        {
          String tmpString = deviceTemplate[x];
          if (tmpString.length())
          {
            String newString = P200_parseTemplate(tmpString, Plugin_200_cols);
            tft.fillScreen(TFT_WHITE);
            if (x==0){
           tft.drawString(newString,0,50);
            }
            if (x==1){
           tft.drawString(newString,0,100);
            }
          }
        }
        success = false;
        break;
      }
  }
  return success;
}

String P200_parseTemplate(String &tmpString, byte lineSize) {
  String result = parseTemplate_padded(tmpString, lineSize);
  const char degree[3] = {0xc2, 0xb0, 0};  // Unicode degree symbol
  const char degree_lcd[2] = {0xdf, 0};  // P012_LCD degree symbol
  result.replace(degree, degree_lcd);

  char unicodePrefix = 0xc3;
  if (result.indexOf(unicodePrefix) != -1) {
    // See: https://github.com/letscontrolit/ESPEasy/issues/2081

    const char umlautAE_uni[3] = {0xc3, 0x84, 0}; // Unicode Umlaute AE
    const char umlautAE_lcd[2] = {0xe1, 0}; // P012_LCD Umlaute
    result.replace(umlautAE_uni, umlautAE_lcd);

    const char umlaut_ae_uni[3] = {0xc3, 0xa4, 0}; // Unicode Umlaute ae
    result.replace(umlaut_ae_uni, umlautAE_lcd);

    const char umlautOE_uni[3] = {0xc3, 0x96, 0}; // Unicode Umlaute OE
    const char umlautOE_lcd[2] = {0xef, 0}; // P012_LCD Umlaute
    result.replace(umlautOE_uni, umlautOE_lcd);

    const char umlaut_oe_uni[3] = {0xc3, 0xb6, 0}; // Unicode Umlaute oe
    result.replace(umlaut_oe_uni, umlautOE_lcd);

    const char umlautUE_uni[3] = {0xc3, 0x9c, 0}; // Unicode Umlaute UE
    const char umlautUE_lcd[2] = {0xf5, 0}; // P012_LCD Umlaute
    result.replace(umlautUE_uni, umlautUE_lcd);

    const char umlaut_ue_uni[3] = {0xc3, 0xbc, 0}; // Unicode Umlaute ue
    result.replace(umlaut_ue_uni, umlautUE_lcd);

    const char umlaut_sz_uni[3] = {0xc3, 0x9f, 0}; // Unicode Umlaute sz
    const char umlaut_sz_lcd[2] = {0xe2, 0}; // P012_LCD Umlaute
    result.replace(umlaut_sz_uni, umlaut_sz_lcd);
  }
  return result;
}

#endif // USES_P012
