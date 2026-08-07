/*
  LED ROULETTE GAME - Arduino Nano
  ---------------------------------
  - Press button -> ring spins counter-clockwise, slows down, stops on a
    random color (red/green/blue/yellow).
  - The matching discrete indicator LED then blinks to show the result.
  - Press the SAME button again -> resets and spins again.

  Hardware:
    D2  - Push button (other leg to GND, uses INPUT_PULLUP)
    D3  - Buzzer
    D4  - NeoPixel ring data pin
    D5  - Red indicator LED
    D6  - Green indicator LED
    D7  - Blue indicator LED
    D8  - Yellow indicator LED
    A0  - left unconnected (random seed noise source)

  Library needed: Adafruit NeoPixel (install via Library Manager)
*/

#include <Adafruit_NeoPixel.h>

// ---------- CONFIG ----------
#define LED_PIN     4
#define NUM_LEDS    16     // MUST be a multiple of 4. Increase freely (24, 32, 40...)
#define BUTTON_PIN  2
#define BUZZER_PIN  3
#define RED_LED     5
#define GREEN_LED   6
#define BLUE_LED    7
#define YELLOW_LED  8

// If your physical ring's index order runs CLOCKWISE instead of
// counter-clockwise, set this to true to flip the spin direction.
#define REVERSE_DIRECTION false

Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

enum Color { RED, GREEN, BLUE, YELLOW };

bool gameRunning = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ring.begin();
  ring.show();

  // seed randomness from an unconnected analog pin (floating noise)
  randomSeed(analogRead(A0));
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(30); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      while (digitalRead(BUTTON_PIN) == LOW) { /* wait for release */ }
      if (!gameRunning) {
        spinWheel();
      }
    }
  }
}

// ---------- helpers ----------

Color colorAt(int index) {
  // Quadrant layout (NUM_LEDS split into 4 equal blocks going around the ring):
  //   Quadrant 1 -> YELLOW
  //   Quadrant 2 -> BLUE
  //   Quadrant 3 -> GREEN
  //   Quadrant 4 -> RED
  int quadrantSize = NUM_LEDS / 4;
  int quadrant = index / quadrantSize;

  switch (quadrant) {
    case 0: return YELLOW;
    case 1: return BLUE;
    case 2: return GREEN;
    case 3: return RED;
  }
  return RED; // fallback, shouldn't be reached
}

uint32_t colorValue(Color c) {
  switch (c) {
    case RED:    return ring.Color(255, 0, 0);
    case GREEN:  return ring.Color(0, 255, 0);
    case BLUE:   return ring.Color(0, 0, 255);
    case YELLOW: return ring.Color(255, 150, 0);
  }
  return 0;
}

int indicatorPin(Color c) {
  switch (c) {
    case RED:    return RED_LED;
    case GREEN:  return GREEN_LED;
    case BLUE:   return BLUE_LED;
    case YELLOW: return YELLOW_LED;
  }
  return -1;
}

void clearAll() {
  ring.clear();
  ring.show();
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  noTone(BUZZER_PIN);
}

// ---------- main game logic ----------

void spinWheel() {
  gameRunning = true;
  clearAll();

  // Reseed with live entropy each spin so results aren't predictable/repeating
  randomSeed(analogRead(A0) + micros());

  int winnerIndex   = random(0, NUM_LEDS);       // truly random stopping LED
  int extraRotations = random(3, 6);              // 3-5 full loops before landing
  int totalSteps      = extraRotations * NUM_LEDS + winnerIndex;

  for (int step = 0; step <= totalSteps; step++) {
    int idx = step % NUM_LEDS;
    if (REVERSE_DIRECTION) idx = (NUM_LEDS - idx) % NUM_LEDS;

    ring.clear();
    ring.setPixelColor(idx, colorValue(colorAt(idx)));
    ring.show();

    tone(BUZZER_PIN, 1000, 15); // roulette "tick" sound

    // Ease-out: fast at first, slows down near the end (cubic curve)
    float progress = (float)step / (float)totalSteps;
    int delayTime = 35 + (int)(pow(progress, 3) * 280); // 35ms -> ~315ms
    delay(delayTime);
  }

  // Land exactly on the winner
  int finalIdx = winnerIndex;
  if (REVERSE_DIRECTION) finalIdx = (NUM_LEDS - winnerIndex) % NUM_LEDS;
  ring.clear();
  ring.setPixelColor(finalIdx, colorValue(colorAt(winnerIndex)));
  ring.show();

  Color winner = colorAt(winnerIndex);
  announceResult(winner);

  gameRunning = false; // pressing the button now restarts the game
}

void announceResult(Color winner) {
  // little "win" fanfare
  int melody[] = {1200, 1500, 1800};
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    delay(180);
  }
  noTone(BUZZER_PIN);

  // blink the matching indicator LED
  int pin = indicatorPin(winner);
  for (int i = 0; i < 6; i++) {
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(200);
  }
  // winning ring LED stays lit as a reminder of the result
  // until the button is pressed again to spin a new round
}
