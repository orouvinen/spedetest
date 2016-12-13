# Spedetesti

Spedetesti is a reaction game, that you can build with Arduino,
where you have to push buttons corresponding to LEDS in the order they light up.

The challenge comes from the phase of the LEDs getting faster and faster.
Each correct button press gives one point and a mistake means game over.

The components laid out on a breadboard:
![BB](spedepeli_bb.png?raw=true)

- 4x LED
- 4x 220ohm resistor
- 4 push buttons
- piezo speaker

## How to play
When powered up, the game is in "main menu".

- To play: press the two rightmost buttons down at the same time

## Hiscores
Highest score will be saved to the EEPROM so it persists across power cycles.
When getting the highest score, you'll see the game asking for your name.
Buttons 1 and 2 (the leftmost buttons) cycle characters left and right.
Button 3 moves on to next position (cursor will wrap back to character position
0 from the last character). Button 4 saves the name.

