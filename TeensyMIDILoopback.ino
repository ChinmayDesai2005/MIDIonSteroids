#include <USBHost_t36.h>
#include <Encoder.h>
#include <U8g2lib.h>
#include "bitmaps.h"
#include "fonts.h"
#include "helpers.h"
#include "constants.h"

// --- USB Host / Device setup ---
USBHost myusb;
MIDIDevice_BigBuffer midiDevice(myusb);

Encoder knob(ENCODER_A, ENCODER_B);

// --- OLED (SH1106) ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

int currentParam = 1; // Expression

// --- State ---
long lastPosition = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

float smoothedPot = 0.0f;
bool potArmed = false;
int lastSentValue = -1;
int beforeDisarmingValue = -1;

int currentChannel = 0;
char channelBuffer[20];

int currentScreen = 1;

// --- UI ---
void drawLegacyScreen() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 8);
  sprintf(channelBuffer, "Channel No. : %d", currentChannel + 1);
  display.print(channelBuffer);

  display.setFont(u8g2_font_fub11_tf);
  display.setCursor(0, 32);
  display.print(params[currentParam].name);

  char ccstr[16];
  sprintf(ccstr, "CC %d", params[currentParam].cc);
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 48, ccstr);

  int val = params[currentParam].value;
  int barWidth = map(val, 0, 127, 0, 120);
  display.drawFrame(4, 54, 120, 8);
  display.drawBox(4, 54, barWidth, 8);

  char valstr[8];
  sprintf(valstr, "%3d", val);
  display.setCursor(100, 48);
  display.print(valstr);

  display.sendBuffer();
}


void drawEffectsScreen() {
  display.clearBuffer();

  int val = params[currentParam].value;

  int pot_pos = map(127 - val, 0, 127, 6, 50);
  display.drawXBMP(111, 2, 16, 60, pot_channel_bitmap_);
  display.drawXBMP(116, pot_pos, 8, 7, pot_knob_bitmap_);

  display.setFont(BoldPixelsSmol);
  char *truncatedName = truncate(params[currentParam].name, 13).c_str();
	display.drawStr(1, 10, truncatedName);
	int stringwidth = display.getStrWidth(truncatedName);
	display.drawBox(1, 12, stringwidth, 1);

  char valstr[8];
  itoa(val, valstr, 10);
  display.setFont(BoldPixels);
  display.drawStr(1, 44, valstr);


  sprintf(channelBuffer, "CH%02d", currentChannel + 1);
	display.setFont(EditUndo);
  display.drawStr(1, 59, channelBuffer);

  display.sendBuffer();
}


void drawScreen(int screen_id = currentScreen) {
  switch (screen_id){
    case 1:
      drawEffectsScreen();
      break;

    default:
      drawLegacyScreen();
      break;
  }
}


void sendParamToCasio(int idx) {
  midiDevice.sendControlChange(params[idx].cc, params[idx].value, currentChannel + 1);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000);
  Serial.println("🎹 Teensy MIDI Bridge (PC→Casio + Casio Loopback + 9 CCs)");

  myusb.begin();
  pinMode(ENCODER_SW, INPUT_PULLUP);

  display.begin();

  smoothedPot = analogRead(POT_PIN);

  for (int i = 0; i < PARAM_COUNT; i++) {
    sendParamToCasio(i);
    delay(5);
  }

  drawScreen();
}

int readSmoothedPotRaw() {
  int raw = analogRead(POT_PIN);
  if (INVERT_POT) raw = 1023 - raw;

  int cc = map(raw, 0, 1023, 0, 127);
  cc = constrain(cc, 0, 127);
  return cc;
}

void disarmPot(int potValue){
  beforeDisarmingValue = potValue;
  potArmed = false;
}



void loop() {
  myusb.Task();

  // // --- Encoder rotation ---
  // long newPos = knob.read() / 4;
  // if (newPos != lastPosition) {
  //   int delta = newPos - lastPosition;
  //   lastPosition = newPos;

  //   int v = params[currentParam].value + delta;
  //   v = constrain(v, 0, 127);
  //   if (v != params[currentParam].value) {
  //     params[currentParam].value = v;
  //     sendParamToCasio(currentParam);
  //     Serial.printf("CC%d (%s) -> %d\n", params[currentParam].cc, params[currentParam].name, v);
  //     drawScreen();
  //   }
  // }

  // Read the Potentiometer to change values
  int potCC = readSmoothedPotRaw();

  // Read the encoder to change pages
  long newPos = knob.read() / 4;
  if (newPos != lastPosition) {
    int delta = newPos - lastPosition;
    lastPosition = newPos;

    disarmPot(potCC);
    currentParam = (currentParam + PARAM_COUNT + delta) % PARAM_COUNT;
    Serial.printf("Changing screens Delta:%d CurrentScreen:%s currentParam:%d\n", delta, params[currentParam].name, currentParam);
    drawScreen();
  }


  if (!potArmed) {
    // If pot is close enough to the current parameter, arm it (no jump)
    // if (abs(potCC - params[currentParam].value) <= POT_PICKUP_THRESHOLD) {
    //   potArmed = true;
    //   Serial.println("Pot armed for parameter.");
    //   lastSentValue = params[currentParam].value;
    // }

    if (abs(beforeDisarmingValue - potCC) > POT_MOVE_THRESHOLD) {
      potArmed = true;
    //   Serial.println("Pot moved enough, enabling.");
    }

  }

  // Armed -> actually update if moved enough
  if (potArmed && abs(potCC - lastSentValue) > 0) {
    params[currentParam].value = potCC;
    lastSentValue = potCC;
    sendParamToCasio(currentParam);
    drawScreen();
    // Serial.printf("Pot -> %s = %d\n", params[currentParam].name, potCC);
  }

  // --- Short press to cycle parameter ---
  bool buttonState = digitalRead(ENCODER_SW);
  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastDebounceTime > debounceDelay) {
    disarmPot(potCC);
    currentChannel = (currentChannel + 1) % MAX_CHANNEL_COUNT;
    Serial.printf("🔁 Switched channel (%d)\n", currentChannel + 1);
    drawScreen();
    lastDebounceTime = millis();
  }
  lastButtonState = buttonState;

  // --- Casio → Teensy → Casio loopback ---
  while (midiDevice.read()) {
    byte type = midiDevice.getType();
    byte ch = midiDevice.getChannel();
    byte d1 = midiDevice.getData1();
    byte d2 = midiDevice.getData2();
    int chInt = ch;

    switch (type) {
      case usbMIDI.NoteOn:
        midiDevice.sendNoteOn(d1, d2, ch);
        Serial.printf("Note On @ D1:%d D2:%d, CH:%d\n", d1, d2, ch);
        break;
      case usbMIDI.NoteOff:        midiDevice.sendNoteOff(d1, d2, ch); break;
      case usbMIDI.ControlChange:  midiDevice.sendControlChange(d1, d2, ch); break;
      case usbMIDI.ProgramChange:
        midiDevice.sendProgramChange(d1, ch);
        Serial.printf("Channel: %d\n", chInt);
        break;
      case usbMIDI.PitchBend:      midiDevice.sendPitchBend(((d2 << 7) | d1) - 8192, ch); break;
      default: break;
    }
  }

  // --- PC → Casio passthrough ---
  while (usbMIDI.read()) {
    byte type = usbMIDI.getType();
    byte ch = usbMIDI.getChannel();
    byte d1 = usbMIDI.getData1();
    byte d2 = usbMIDI.getData2();

    switch (type) {
      case usbMIDI.NoteOn:         midiDevice.sendNoteOn(d1, d2, ch); break;
      case usbMIDI.NoteOff:        midiDevice.sendNoteOff(d1, d2, ch); break;
      case usbMIDI.ControlChange:  midiDevice.sendControlChange(d1, d2, ch); break;
      case usbMIDI.ProgramChange:  midiDevice.sendProgramChange(d1, ch); break;
      case usbMIDI.PitchBend:      midiDevice.sendPitchBend(((d2 << 7) | d1) - 8192, ch); break;
      case usbMIDI.SystemExclusive:
        Serial.println("SysEx Detected!");
        {
          const uint8_t* data = usbMIDI.getSysExArray();
          uint32_t length = usbMIDI.getSysExArrayLength();
        
          Serial.print("Length: ");
          Serial.println(length);

          Serial.print("Data: ");

          if (length > 0 && data != nullptr) {
              // Print hex bytes
              for (uint32_t i = 0; i < length; i++) {
                  if (data[i] < 16) Serial.print('0'); // leading zero for alignment
                  Serial.print(data[i], HEX);
                  Serial.print(' ');
              }
              Serial.println();

              // Forward exactly as received
              midiDevice.sendSysEx(length, data, true);

              Serial.println("PC->Casio SysEx forwarded.");
          }
        }
        break;

      default: break;
    }
  }

  delay(2);
}


// TODO: Add Functionality
// 1. Portamento Settings
// 2. Vibrato Settings
// 3. Mixers Page
// 4. Add assignables and routing