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
#define TFT_BL              4   // Display backlight control pin

#define P200_Nlines 4        // The number of different lines which can be displayed
#define P200_Nchars 80

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
        String options2[2];
        options2[0] = F("0");
        options2[1] = F("1");
        int optionValues2[2] = { 0, 1 };
        addFormSelector(F("Rotat"), F("p200_rotat"), 2, options2, optionValues2, choice1);

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
        String options2[7];
        options2[0] = F("glcdfont");
        options2[1] = F("Font7srle");
        options2[2] = F("Font16");
        options2[3] = F("Font32rle");
        options2[4] = F("Font64rle");
        options2[5] = F("Font72rle");
        options2[6] = F("Font72x53rle");
        int optionValues2[7] = { 1, 2, 3, 4, 5, 6, 7 };
        addFormSelector(F("Font"), F("p200_font"), 7, options2, optionValues2, choice3);

        {
          String strings[P200_Nlines];
          LoadCustomTaskSettings(event->TaskIndex, strings, P200_Nlines, P200_Nchars);
          for (byte varNr = 0; varNr < P200_Nlines; varNr++)
          {
            addFormTextBox(String(F("Line ")) + (varNr + 1), getPluginCustomArgName(varNr), strings[varNr], P200_Nchars);
          }
        }

        addFormNumericBox(F("Display Timeout"), F("p200_timer"), PCONFIG(3));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p200_rotat"));
        PCONFIG(1) = getFormItemInt(F("p200_datun"));
        PCONFIG(2) = getFormItemInt(F("p200_font"));
        PCONFIG(3) = getFormItemInt(F("p200_timer"));

        // FIXME TD-er: This is a huge stack allocated object.
        char deviceTemplate[P12_Nlines][P12_Nchars];
        String error;
        for (byte varNr = 0; varNr < P12_Nlines; varNr++)
        {
          if (!safe_strncpy(deviceTemplate[varNr], web_server.arg(getPluginCustomArgName(varNr)), P12_Nchars)) {
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
        if (PCONFIG(1) == 2) {
          Plugin_012_rows = 4;
          Plugin_012_cols = 20;
        } else if (PCONFIG(1) == 1) {
          Plugin_012_rows = 2;
          Plugin_012_cols = 16;
        }

        Plugin_012_mode = PCONFIG(3);

        //TODO:LiquidCrystal_I2C class doesn't have destructor. So if LCD type (size) is changed better reboot for changes to take effect.
        // workaround is to fix the cols and rows at its maximum (20 and 4)
        if (!lcd)
          lcd = new LiquidCrystal_I2C(PCONFIG(0), 20, 4); //Plugin_012_cols, Plugin_012_rows);

        // Setup LCD display
        lcd->init();                      // initialize the lcd
        lcd->backlight();
        lcd->print(F("ESP Easy"));
        displayTimer = PCONFIG(2);
        if (CONFIG_PIN3 != -1)
          pinMode(CONFIG_PIN3, INPUT_PULLUP);
        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        if (CONFIG_PIN3 != -1)
        {
          if (!digitalRead(CONFIG_PIN3))
          {
            if (lcd) {
              lcd->backlight();
            }
            displayTimer = PCONFIG(2);
          }
        }
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if ( displayTimer > 0)
        {
          displayTimer--;
          if (lcd && displayTimer == 0)
            lcd->noBacklight();
        }
        break;
      }

    case PLUGIN_READ:
      {
        // FIXME TD-er: This is a huge stack allocated object.
        char deviceTemplate[P12_Nlines][P12_Nchars];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        for (byte x = 0; x < Plugin_012_rows; x++)
        {
          String tmpString = deviceTemplate[x];
          if (lcd && tmpString.length())
          {
            String newString = P012_parseTemplate(tmpString, Plugin_012_cols);
            lcd->setCursor(0, x);
            lcd->print(newString);
          }
        }
        success = false;
        break;
      }

    case PLUGIN_WRITE:
      {
        String cmd = parseString(string, 1);
        if (lcd && cmd.equalsIgnoreCase(F("LCDCMD")))
        {
          success = true;
          String arg1 = parseString(string, 2);
          if (arg1.equalsIgnoreCase(F("Off"))){
              lcd->noBacklight();
          }
          else if (arg1.equalsIgnoreCase(F("On"))){
              lcd->backlight();
          }
          else if (arg1.equalsIgnoreCase(F("Clear"))){
              lcd->clear();
          }
        }
        else if (lcd && cmd.equalsIgnoreCase(F("LCD")))
        {
          success = true;
          int colPos = event->Par2 - 1;
          int rowPos = event->Par1 - 1;
          String text = parseStringKeepCase(string, 4);
          text = P012_parseTemplate(text, Plugin_012_cols);

          //clear line before writing new string
          if (Plugin_012_mode == 2){
              lcd->setCursor(colPos, rowPos);
              for (byte i = colPos; i < Plugin_012_cols; i++) {
                  lcd->print(" ");
              }
          }

          // truncate message exceeding cols
          lcd->setCursor(colPos, rowPos);
          if(Plugin_012_mode == 1 || Plugin_012_mode == 2){
              lcd->setCursor(colPos, rowPos);
              for (byte i = 0; i < Plugin_012_cols - colPos; i++) {
                  if(text[i]){
                     lcd->print(text[i]);
                  }
              }
          }

          // message exceeding cols will continue to next line
          else{
              // Fix Weird (native) lcd display behaviour that split long string into row 1,3,2,4, instead of 1,2,3,4
              boolean stillProcessing = 1;
              byte charCount = 1;
              while(stillProcessing) {
                   if (++colPos > Plugin_012_cols) {    // have we printed 20 characters yet (+1 for the logic)
                        rowPos += 1;
                        lcd->setCursor(0,rowPos);   // move cursor down
                        colPos = 1;
                   }

                   //dont print if "lower" than the lcd
                   if(rowPos < Plugin_012_rows  ){
                       lcd->print(text[charCount - 1]);
                   }

                   if (!text[charCount]) {   // no more chars to process?
                        stillProcessing = 0;
                   }
                   charCount += 1;
              }
              //lcd->print(text.c_str());
              // end fix
          }

        }
        break;
      }

  }
  return success;
}

// Perform some specific changes for LCD display
// https://www.letscontrolit.com/forum/viewtopic.php?t=2368
String P012_parseTemplate(String &tmpString, byte lineSize) {
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
