#include <LiquidCrystal.h>
#include <EEPROM.h>

/*
 * I/O pins
 */
// Use analog pins from A0-A3 as regular GPIO pins for feeding the LEDs
const int ledPins[] = {14, 15, 16, 17};
// Button pins for buttons from left to right
const int btnPins[] = {8, 9, 10, 11};

// Button functions for entering name tag for hiscore
#define BUTTON_VALUE_DOWN 0  // Scroll to next letter in alphabet
#define BUTTON_VALUE_UP   1  // Scroll to previous letter in alphabet
#define BUTTON_NEXT       2  // Move on to next character
#define BUTTON_ENTER      3  // Confirm
#define NO_BUTTON -1

/*
 * Timing
 */
const int initDelay = 1000;  /* initial delay in ms */
const int delayDec  = 20;    /* delay decrement (ms) after each LED */
const int minDelay  = 300;   /* final delay */
unsigned long btnDelay = initDelay;
unsigned long nextLedTime = 0;

/*
 * Queue for storing led order
 */
#define QUEUE_MAX 256
int ledQueue[QUEUE_MAX];
int queueSize  = 0;
int queueBegin = 0;
int queueEnd   = 0;

/*
 * The button that was last processed.
 * This variable is used to prevent processing a button
 * being held down as multiple clicks on the same button
 */
int lastButton = -1;

enum state { STOPPED, RUNNING };
int gameState = STOPPED;
unsigned short score = 0;

#define EEPROM_HISCORE_ADDR 0x100
#define HISCORE_HEADER 0x0771
struct hiscore {
     unsigned short header;
     unsigned short score;
     char nameTag[8+1];
} hiscore;

// LCD init
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup()
{
     int i;

     // Initialise I/O pins
     for (i = 0; i < 4; i++) {
          pinMode(ledPins[i], OUTPUT);
          pinMode(btnPins[i], INPUT_PULLUP);
     }
     lcd.begin(20, 4);
     lcd.setCursor(0, 0);

     // Initialise random number generator
     randomSeed(analogRead(5));
     resetGame();
}

void setLedState(int led, int state)
{
     digitalWrite(ledPins[led], state);
}

/*
 * Main loop
 */
void loop()
{
     unsigned long now = millis();
     static bool showingWaitingMessage = false;

     if (gameState == STOPPED) {
          if (!showingWaitingMessage) {
               printWaitingMessage();
               showingWaitingMessage = true;
          }

          // Press two rightmost buttons to start the game
          if (buttonPressed(2) && buttonPressed(3)) {
               gameState = RUNNING;
               // make sure the message prints after the game is over
               showingWaitingMessage = false;

               resetGame();
               nextLedTime = now + btnDelay;
          }
          return;
     }

     // Check if it's time to light up a new LED
     if (now >= nextLedTime) {
          int nextLed;
          int currentLed;

          // Set current LED off (if this is not the first led)
          if (queueSize > 0) {
               currentLed = ledQueue[queueEnd - 1];
               setLedState(currentLed, LOW);
          } else
               currentLed = lastButton; // Will be -1 if this is the first LED

          // Get new LED number
          do {
               nextLed = random(4);
          } while (nextLed == currentLed);

          // Save led to queue
          if (!queue_push(nextLed))
               resetGame(); // Player fell asleep, queue full

          setLedState(nextLed, HIGH);

          // Increase LED phase
          if (btnDelay > minDelay)
               btnDelay -= delayDec;
          nextLedTime = now + btnDelay;
     }
     handleInput();
}

void printEepromHiscore(void)
{
     EEPROM.get(EEPROM_HISCORE_ADDR, hiscore);
     if (hiscore.header != HISCORE_HEADER) {
          strcpy(hiscore.nameTag, "");
          lcd.print("--");
     } else {
          lcd.print(hiscore.nameTag);
          lcd.print(": ");
          lcd.print(hiscore.score);
     }
}


// Print score during game
void printScore(void)
{
     lcd.setCursor(0, 0);
     lcd.print(score);
}

// Main screen
void printWaitingMessage(void)
{
     lcd.clear();
     lcd.setCursor(0, 0); lcd.print("Paras tulos:");
     lcd.setCursor(0, 1); printEepromHiscore();
     lcd.setCursor(0, 3); lcd.print("Napista lahtee...");
}


// Input handling during game
void handleInput(void)
{
     int button;

     /* Avoid reading the "game start" click when no leds have yet been lit up */
     if (queueEnd == 0)
          return;

     if ((button = getPressedButton()) != NO_BUTTON) {
          int led = queue_pop();
          // Don't count it as a mistake to pre-click
          if (led == -1)
               return;
          if (led != button)
               gameOver();
          else {
               score++;
               lastButton = button;
               setLedState(led, LOW);
               printScore();
          }
     }
}


// Checks if any of the buttons have been pressed.
// If a button has been pressed, returns the button number (0-3).
// Returns -1 if no button was pressed.
int getPressedButton()
{
     for (int i = 0; i < (signed)(sizeof(btnPins) / sizeof(btnPins[0])); i++) {
          // Handle pressed button
          // (but avoid accidental "double" click processing)
          if (buttonPressed(i)) {
               if (lastButton == i)
                    return NO_BUTTON;

               lastButton = i;
               return i;
          }
     }
     lastButton = NO_BUTTON;
     return NO_BUTTON;
}


void gameOver()
{
     int i;

     gameState = STOPPED;

     // Flash the LEDs once
     for (i = 0; i < 4; i++)
          digitalWrite(ledPins[i], HIGH);

     delay(1000);

     for (i = 0; i < 4; i++)
          digitalWrite(ledPins[i], LOW);

     lcd.clear();

     // Handle possible hiscore
     if (score > hiscore.score) {
          getHiscoreNameTag();
          hiscore.score = score;
          hiscore.header = HISCORE_HEADER;
          EEPROM.put(EEPROM_HISCORE_ADDR, hiscore);
     } else {
          lcd.setCursor(0, 0); lcd.print("Voi rahma!");
          lcd.setCursor(0, 1); lcd.print("Pisteet: ");
          lcd.print(score);
          delay(5000);
     }
}


void getHiscoreNameTag(void)
{
     lcd.clear();
     strcpy(hiscore.nameTag, "");
     adjustStringValue("VOEJMAHOTON!", "ANNA NIMI:", hiscore.nameTag, 8);
}

// Prompts user for string input.
// Before input field, two lines of headings can be printed (heading and
// subheading).
// Third argument is pointer to the destination string, which must be able to
// hold maxLength characters plus a terminating nul char.
void adjustStringValue(
          const char *heading,
          const char *subHeading,
          char *string,
          int maxLength)
{
     lcd.clear();
     lcd.print(heading);
     lcd.setCursor(0, 1);
     lcd.print(subHeading);

     // If empty destination string was passed,
     // then initialize it with 'A' so there's something
     // to show to the user
     if (string[0] == '\0')
          string[0] = 'A';

     lcd.setCursor(0, 2);
     lcd.print(string);
     lcd.setCursor(0, 2);
     lcd.cursor();
     lcd.blink();

     int i = 0;
     for (;;) {
          int button = getPressedButton();

          if (button == BUTTON_VALUE_UP) {
               // Order of these tests matter
               if (string[i] == ' ')
                    string[i] = 'A';
               else if (string[i] == 'Z')
                    string[i] = ' ';
               else
                    string[i]++;
          }
          if (button == BUTTON_VALUE_DOWN) {
               // Likewise, the order matters
               if (string[i] == ' ')
                    string[i] = 'Z';
               else if (string[i] == 'A')
                    string[i] = ' ';
               else
                    string[i]--;
          }
          if (button == BUTTON_NEXT) {
               i = (i + 1) % maxLength;
               // When moving onto next letter, initialise with the letter the
               // user typed last. Also allow copying of spaces for quick string
               // tail deletion.
               if (string[i] == '\0' || (i && string[i-1] == ' '))
                    string[i] = string[i-1];
          }
          if (button != NO_BUTTON) {
               lcd.setCursor(0, 2);
               lcd.print(string);
               lcd.setCursor(i, 2);
          }
          if (button == BUTTON_ENTER)
               break;
     }
     lcd.noCursor();
     lcd.noBlink();
     lcd.clear();
}


void resetGame()
{
     int i;

     // LEDs off
     for (i = 0; i < 4; i++)
          digitalWrite(ledPins[i], LOW);

     // Reset LED queue and return to inital LED interval
     queueReset();
     btnDelay = initDelay;
     lastButton = -1;
     score = 0;
     lcd.clear();
}


void queueReset(void)
{
     int i;
     for (i = 0; i < QUEUE_MAX; i++)
          ledQueue[i] = -1;

     queueBegin = 0;
     queueEnd = 0;
     queueSize = 0;
}


int queue_push(int x)
{
     if (queueSize == QUEUE_MAX)
          return 0; /* Queue is full */

     ledQueue[queueEnd++] = x;

     if (queueEnd >= QUEUE_MAX)
          queueEnd = 0;
     queueSize++;
     return 1;
}


int queue_pop(void)
{
     int event;

     if (queueSize == 0)
          return -1;  /* Queue empty, cannot pop */

     event = ledQueue[queueBegin++];

     if (queueBegin >= QUEUE_MAX)
          queueBegin = 0;
     queueSize--;

     return event;
}


bool buttonPressed(int button)
{
     return !digitalRead(btnPins[button]);
}
