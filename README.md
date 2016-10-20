# Spedetesti

Spedetesti is a reaction game, that you can build with Arduino,
where you have to push buttons corresponding to LEDS in the order they light up.

The challenge comes from the phase of the LEDs getting faster and faster.
Each correct button press gives one point and a mistake means game over.

The components laid out on a breadboard:
![BB](spedepeli_bb.png?raw=true)

- 4x 220ohm resistor
- 4 push buttons
- piezo speaker

## How to play
When powered up, the game is in "main menu".

- To play: press the two rightmost buttons down at the same time
- To hear last score: -"- leftmost buttons -"-

The score is reported by a series of beeps from the piezo speaker.
One long beep for each 10 points, and after that one short beep for each single point.
For example a score of 23 would "beeep beeep   bep bep bep".
