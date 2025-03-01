# Arduino Dual-Mode Stopwatch and Countdown Timer

A versatile Arduino project that implements both a stopwatch with lap functionality and a countdown timer with alert notification.

## Features

- **Dual functionality:** Switch between stopwatch and timer modes
- **Stopwatch Mode**
  - Start/stop time measurement
  - Record and display lap times
  - Store up to 10 laps with cycling memory
  - Display format: H:MM:SS.s
- **Timer Mode**
  - Set countdown time in minutes:seconds
  - Add time in 10-second increments
  - Visual and audio alert when countdown reaches zero
  - Display format: MM:SS
- **User interface**
  - 16x2 LCD display for time and status information
  - Button debouncing for reliable input
  - Long/short press detection for mode switching
  - Serial debugging capabilities

## Hardware Requirements

- Arduino Mega 2560
- 16x2 LCD Display
- 3 Pushbuttons
- Buzzer module
- 4x 10kΩ resistors (for button pull-down)
- Jumper wires and breadboard

## Wiring Diagram

Connect components as follows:

### LCD 16x2
- VSS → GND
- VDD → 5V
- V0 → 10kΩ resistor → GND (contrast control)
- RS → Pin 12
- E → Pin 11
- D4-D7 → Pins 5, 4, 3, 2
- A (Backlight+) → 5V
- K (Backlight-) → GND

### Buttons
- Start/Stop button → Pin 7
- Reset button → Pin 8
- Lap button → Pin 9
- Each button needs a 10kΩ pull-down resistor to GND

### Buzzer
- Positive terminal → Pin 10
- Negative terminal → GND

## Dependencies

- [LiquidCrystal](https://www.arduino.cc/en/Reference/LiquidCrystal) - For LCD control
- [Bounce2](https://github.com/thomasfredericks/Bounce2) - For button debouncing

## Installation

1. Install the required libraries in Arduino IDE
2. Connect hardware according to the wiring diagram
3. Upload the code to your Arduino Mega

## Usage

### Stopwatch Mode
- **Start/Stop**: Press the Start/Stop button to begin measuring time; press again to pause
- **Record Lap**: While running, press the Lap button to record a lap time
- **Reset**: Short press the Reset button to clear all times

### Timer Mode
- **Enter Timer Mode**: Long press (≥0.5s) the Reset button
- **Set Time**: Press the Lap button to add 10 seconds per press
- **Start/Pause**: Press the Start/Stop button to start countdown; press again to pause
- **Reset**: Short press the Reset button to clear the timer
- **Return to Stopwatch**: Long press the Reset button

### Alert
When the timer reaches 00:00, the display blinks "TIME'S UP!" and the buzzer sounds for 5 seconds.

## Code Structure

The project code is organized into several logical sections:
- Configuration and initialization
- Debouncing button management
- Stopwatch logic
- Timer logic
- Reset button logic (short/long press)
- Helper functions (formatting time, lap management)
- Buzzer handling

## Example Use Cases

- Using the stopwatch to time exercise sessions with lap tracking
- Setting a countdown for cooking, presentations, or work intervals
- Using the lap function to time multiple repetitions during training

## Author

Tcaci Liviu

## License

This project is open source and available under the MIT License.
