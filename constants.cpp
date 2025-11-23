#include <Arduino.h>

// --- Teensy Pins ---
const int ENCODER_A = 2;
const int ENCODER_B = 3;
const int ENCODER_SW = 4;

const int POT_PIN = A0;
const int POT_PICKUP_THRESHOLD = 5; // CC steps tolerance to arm
const int POT_MOVE_THRESHOLD = 5; 
const bool INVERT_POT = true;

// --- MIDI Mappings ---
struct NameCCMap {
  const String name;
  uint8_t cc;
};

const NameCCMap nameCCmapping[] = {
  // Name,              CC
  {"Mod Wheel",         1},
  {"Expression",        11},
  {"Filter Cutoff",     74},
  {"Filter Resonance",  71},
  {"Attack",            73},
  {"Release",           72},
  {"Reverb Send",       91},
  {"Chorus Send",       93}
};

const int NUM_PARAMS = sizeof(nameCCmapping) / sizeof(nameCCmapping[0]);

struct ParamValue {
  uint8_t cc;
  int value;
};

const ParamValue defaultParamValues[] = {
  //CC,     Default
  {1,      0}, 
  {11,     64},
  {74,     64},
  {71,     64},
  {73,     64},
  {75,     64},
  {70,     64},
  {72,     64},
  {91,     40},
  {93,     40}
};


const int MAX_CHANNEL_COUNT = 2;

const unsigned long debounceDelay = 100;
