// --- Teensy Pins ---
const int ENCODER_A = 2;
const int ENCODER_B = 3;
const int ENCODER_SW = 4;

const int POT_PIN = A0;
const int POT_PICKUP_THRESHOLD = 5; // CC steps tolerance to arm
const int POT_MOVE_THRESHOLD = 5; 
const bool INVERT_POT = true;

// --- MIDI parameters ---
struct Param {
  const String name;
  uint8_t cc;
  int value;
};
Param params[] = {
  {"Mod Wheel",      1,  0},    // Eventually port to assignable
  {"Expression",    11, 64},    // Eventually port to assignable
  {"Filter Cutoff", 74, 64},    // Eventually port to assignable
  {"Filter Resonance", 71, 64 },
  {"Attack",        73, 64},
  // {"Decay",         75, 64},
  // {"Sustain",       70, 64},
  {"Release",       72, 64},
  {"Reverb Send",   91, 40},
  {"Chorus Send",   93, 40}
};

const int PARAM_COUNT = sizeof(params) / sizeof(params[0]);

const int MAX_CHANNEL_COUNT = 2;

const unsigned long debounceDelay = 100;
