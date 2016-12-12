#include <LiquidCrystal.h>
/*
 * I/O pins
 */
const int led_pins[] = {14, 15, 16, 17};  // Use analog pins from A0-A3 as regular GPIO pins for feeding the LEDs
const int btn_pins[] = {8, 9, 10, 11};

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
int last_handled = -1;

enum state { STOPPED, RUNNING };
int game_state = STOPPED;
int score = 0;


LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup()
{
     int i;

     /* Initialise I/O pins */
     for (i = 0; i < 4; i++) {
          pinMode(led_pins[i], OUTPUT);
          pinMode(btn_pins[i], INPUT_PULLUP);
     }
     lcd.begin(20, 4);
     lcd.setCursor(0, 0);

     /* Initialise random number generator */
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

          /* Press two rightmost buttons to start the game */
          if (button_pressed(2) && button_pressed(3)) {
               game_state = RUNNING;
               showing_waiting_message = false; // make sure the message prints aftear the game is over
               reset_game();
               next_led_time = now + btn_delay;
          } else if (button_pressed(0) && button_pressed(1)) {
               /* Report last score with two leftmost buttons */
               report_score();
          }
          return;
     }

     /* Check if it's time to light up a new LED */
     if (now >= next_led_time) {
          int next_led;
          int current_led;

          /* Set current LED off (if this is not the first led) */
          if (queue_size > 0) {
               current_led = led_queue[queue_end - 1];
               set_led_state(current_led, LOW);
          } else
               current_led = last_handled; /* Will be -1 if this is the first LED */

          /* Get new LED number */
          do {
               next_led = random(4);
          } while (next_led == current_led);

          /* Save led to queue */
          if (!queue_push(next_led))
               reset_game(); /* player fell asleep, queue full */

          set_led_state(next_led, HIGH);

          /* Increase LED phase  */
          if (btn_delay > min_delay)
               btn_delay -= delay_dec;
          next_led_time = now + btn_delay;
     }
     handle_input();
}

void print_score(void)
{
     lcd.setCursor(0, 0);
     lcd.print(score);
}


void print_waiting_message(void)
{
     lcd.clear();
     lcd.setCursor(0, 0); lcd.print("Paras tulos:");
     lcd.setCursor(0, 1); lcd.print("=== ORBI "); lcd.print("118 ===");
     lcd.setCursor(0, 3); lcd.print("Napista lahtee...");
}

void handle_input(void)
{
     int i;

     /* Avoid reading the "game start" click when no leds have yet been lit up */
     if (queue_end == 0)
          return;

     for (i = 0; i < 4; i++) {
          /*
           * Handle pressed button (but avoid accidental "double" click processing)
           */
          if (button_pressed(i) && i != last_handled) {
               handle_button(i);
               break;
          }
     }
}


void handle_button(int button)
{
     int led;

     led = queue_pop();
     /* Don't count it as a mistake to try and preclick */
     if (led == -1)
          return;

     if (led != button)
          game_over();
     else {
          score++;
          last_handled = led;
          set_led_state(led, LOW);
          print_score();
     }
}

void game_over()
{
     int i;

     /*
      * Flash the leds once and report score
      */

     game_state = STOPPED;
     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], HIGH);

     delay(1000);

     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], LOW);

     lcd.clear();
     lcd.setCursor(0, 0); lcd.print("Voi rahma!");
     lcd.setCursor(0, 1); lcd.print("Pisteet: ");
     lcd.print(score);
     //lcd.print(score);
     delay(5000);

     //report_score();
}

void report_score()
{
}

void reset_game()
{
     int i;

     /* Set LEDS off */
     for (i = 0; i < 4; i++)
          digitalWrite(led_pins[i], LOW);

     /* Reset LED queue and return to inital LED interval */
     queue_reset();
     btn_delay = init_delay;
     last_handled = -1;
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
