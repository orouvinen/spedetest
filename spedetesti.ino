#include <LiquidCrystal.h>
#include <EEPROM.h>

/*
 * I/O pins
 */
// Use analog pins from A0-A3 as regular GPIO pins for feeding the LEDs
const int led_pins[] = {14, 15, 16, 17};
// Button pins for buttons from left to right
const int btn_pins[] = {8, 9, 10, 11};

// Button functions for entering name tag for hiscore
#define BUTTON_VALUE_DOWN 0  // Scroll to next letter in alphabet
#define BUTTON_VALUE_UP   1  // Scroll to previous letter in alphabet
#define BUTTON_NEXT       2  // Move on to next character
#define BUTTON_ENTER      3  // Confirm
#define NO_BUTTON -1

/*
 * Timing
 */
const int init_delay = 1000;  /* initial delay in ms */
const int delay_dec  = 20;    /* delay decrement (ms) after each LED */
const int min_delay  = 300;   /* final delay */
unsigned long btn_delay = init_delay;
unsigned long next_led_time = 0;

/*
 * Queue for storing led order
 */
#define QUEUE_MAX 256
int led_queue[QUEUE_MAX];
int queue_size  = 0;
int queue_begin = 0;
int queue_end   = 0;

/*
 * The button that was last processed.
 * This variable is used to prevent processing a button
 * being held down as multiple clicks on the same button
 */
int last_button = -1;

enum state { STOPPED, RUNNING };
int game_state = STOPPED;
unsigned short score = 0;

#define EEPROM_HISCORE_ADDR 0x100
#define HISCORE_HEADER 0x0771
struct hiscore {
     unsigned short header;
     unsigned short score;
     char nametag[8+1];
} hiscore;

// LCD init
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup()
{
     int i;
     /*
     hiscore.header = 0;
     hiscore.score = 0;
     strcpy(hiscore.nametag, "        ");
     EEPROM.put(EEPROM_HISCORE_ADDR, hiscore);
     while(1);
     */

     // Initialise I/O pins
     for (i = 0; i < 4; i++) {
          pinMode(led_pins[i], OUTPUT);
          pinMode(btn_pins[i], INPUT_PULLUP);
     }
     lcd.begin(20, 4);
     lcd.setCursor(0, 0);

     // Initialise random number generator
     randomSeed(analogRead(0));
     reset_game();
}

void set_led_state(int led, int state)
{
     digitalWrite(led_pins[led], state);
}

/*
 * Main loop
 */
void loop()
{
     unsigned long now = millis();
     static bool showing_waiting_message = false;

     if (game_state == STOPPED) {
          if (!showing_waiting_message) {
               print_waiting_message();
               showing_waiting_message = true;
          }

          // Press two rightmost buttons to start the game
          if (button_pressed(2) && button_pressed(3)) {
               game_state = RUNNING;
               // make sure the message prints after the game is over
               showing_waiting_message = false;

               reset_game();
               next_led_time = now + btn_delay;
          }
          return;
     }

     // Check if it's time to light up a new LED
     if (now >= next_led_time) {
          int next_led;
          int current_led;

          // Set current LED off (if this is not the first led)
          if (queue_size > 0) {
               current_led = led_queue[queue_end - 1];
               set_led_state(current_led, LOW);
          } else
               current_led = last_button; // Will be -1 if this is the first LED

          // Get new LED number
          do {
               next_led = random(4);
          } while (next_led == current_led);

          // Save led to queue
          if (!queue_push(next_led))
               reset_game(); // Player fell asleep, queue full

          set_led_state(next_led, HIGH);

          // Increase LED phase
          if (btn_delay > min_delay)
               btn_delay -= delay_dec;
          next_led_time = now + btn_delay;
     }
     handle_input();
}

void print_eeprom_hiscore(void)
{
     EEPROM.get(EEPROM_HISCORE_ADDR, hiscore);
     if (hiscore.header != HISCORE_HEADER) {
          strcpy(hiscore.nametag, "");
          lcd.print("--");
     } else {
          lcd.print(hiscore.nametag);
          lcd.print(": ");
          lcd.print(hiscore.score);
     }
}


// Print score during game
void print_score(void)
{
     lcd.setCursor(0, 0);
     lcd.print(score);
}

// Main screen
void print_waiting_message(void)
{
     lcd.clear();
     lcd.setCursor(0, 0); lcd.print("Paras tulos:");
     lcd.setCursor(0, 1); print_eeprom_hiscore();
     lcd.setCursor(0, 3); lcd.print("Napista lahtee...");
}


// Input handling during game
void handle_input(void)
{
     int button;

     /* Avoid reading the "game start" click when no leds have yet been lit up */
     if (queue_end == 0)
          return;

     if ((button = get_pressed_button()) != NO_BUTTON) {
          int led = queue_pop();
          // Don't count it as a mistake to pre-click
          if (led == -1)
               return;
          if (led != button)
               game_over();
          else {
               score++;
               last_button = button;
               set_led_state(led, LOW);
               print_score();
          }
     }
}


// Checks if any of the buttons have been pressed.
// If a button has been pressed, returns the button number (0-3).
// Returns -1 if no button was pressed.
int get_pressed_button()
{
     for (int i = 0; i < sizeof(btn_pins) / sizeof(btn_pins[0]); i++) {
          // Handle pressed button
          // (but avoid accidental "double" click processing)
          if (button_pressed(i)) {
               if (last_button == i)
                    return NO_BUTTON;

               last_button = i;
               return i;
          }
     }
     last_button = NO_BUTTON;
     return NO_BUTTON;
}


void game_over()
{
     int i;

     game_state = STOPPED;

     // Flash the LEDs once
     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], HIGH);

     delay(1000);

     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], LOW);

     lcd.clear();

     // Handle possible hiscore
     if (score > hiscore.score) {
          get_hiscore_nametag();
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


void get_hiscore_nametag(void)
{
     lcd.clear();
     strcpy(hiscore.nametag, "");
     adjustStringValue("VOEJMAHOTON!", "ANNA NIMI:", hiscore.nametag, 8);
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
          int button = get_pressed_button();

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


void reset_game()
{
     int i;

     // LEDs off
     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], LOW);

     // Reset LED queue and return to inital LED interval
     queue_reset();
     btn_delay = init_delay;
     last_button = -1;
     score = 0;
     lcd.clear();
}


void queue_reset(void)
{
     int i;
     for (i = 0; i < QUEUE_MAX; i++)
          led_queue[i] = -1;

     queue_begin = 0;
     queue_end = 0;
     queue_size = 0;
}


int queue_push(int x)
{
     if (queue_size == QUEUE_MAX)
          return 0; /* Queue is full */

     led_queue[queue_end++] = x;

     if (queue_end >= QUEUE_MAX)
          queue_end = 0;
     queue_size++;
     return 1;
}


int queue_pop(void)
{
     int event;

     if (queue_size == 0)
          return -1;  /* Queue empty, cannot pop */

     event = led_queue[queue_begin++];

     if (queue_begin >= QUEUE_MAX)
          queue_begin = 0;
     queue_size--;

     return event;
}


bool button_pressed(int button)
{
     return !digitalRead(btn_pins[button]);
}
