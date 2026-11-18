# Voltnite ESP32 Smart Vending Controller

**Candidate:** Hemant Shakywar\
**Board:** ESP32 DevKit V1\
**Framework:** Arduino\
**Baud rate:** 115200\
**Project type:** Hardware / Wokwi simulation

## 1. Project overview

This project implements a miniature vending controller using an ESP32.
It accepts simulated payment through the Serial Monitor, lets the user
select one of two products using physical push buttons, and completes a
vending transaction only after receiving a success acknowledgement.

The controller maintains a credit balance and uses a finite-state
machine (FSM) to manage the vending process. Product cost is deducted
**only after `VEND_OK` is received**. A failed acknowledgement or
acknowledgement timeout leaves the balance unchanged.

This is a simulation-only exercise. No real payment system, mains
voltage, or dispensing motor is required.

## 2. Features

-   Accepts payment commands `PAY 10` and `PAY 20`.
-   Rejects unsupported payment commands, including negative and invalid
    values.
-   Supports two physical product-selection push buttons.
-   Checks available credit before starting a vend.
-   Implements `IDLE`, `CREDIT_AVAILABLE`, `VENDING`, `SUCCESS`, and
    `FAILED` states.
-   Accepts `VEND_OK` and `VEND_FAIL` acknowledgements through Serial
    Monitor.
-   Uses a 5-second acknowledgement timeout.
-   Deducts product price only after successful acknowledgement.
-   Preserves credit after a failed vend, timeout, or
    insufficient-credit rejection.
-   Uses button debounce and non-blocking `millis()` timing.
-   Reports uptime, credit, selected product, state transitions, and
    errors over Serial.

## 3. Product and payment configuration

  Item                              Value
  ------------------------- -------------
  Product A                        INR 15
  Product B                        INR 25
  Payment command 1              `PAY 10`
  Payment command 2              `PAY 20`
  Success acknowledgement       `VEND_OK`
  Failure acknowledgement     `VEND_FAIL`
  Acknowledgement timeout       5 seconds
  Initial credit                    INR 0

Only the two specified payment commands are accepted. The firmware does
not implement a real payment interface.

## 4. Hardware required

  Component                    Quantity Notes
  ----------------------- ------------- --------------------------------------
  ESP32 DevKit V1                     1 ESP32-WROOM-32 development board
  Momentary push button               2 Product A and Product B selection
  Green LED                           1 Status indication
  Red LED                             1 Vending / failure indication
  220-ohm resistor                    2 One resistor in series with each LED
  Breadboard                          1 For prototyping
  Jumper wires              As required Male-to-male
  USB data cable                      1 Programming and power

The ESP32 is powered through USB. Use a low-voltage DC setup only.

## 5. Updated wiring and GPIO assignment

The following pin mapping matches the updated firmware:

  -----------------------------------------------------------------------
  ESP32 GPIO              Component               Connection
  ----------------------- ----------------------- -----------------------
  GPIO 25                 Product A button        Button between GPIO 25
                                                  and GND

  GPIO 26                 Product B button        Button between GPIO 26
                                                  and GND

  GPIO 19                 Green status LED        GPIO 19 → 220 Ω
                                                  resistor → LED anode;
                                                  LED cathode → GND

  GPIO 18                 Red vending LED         GPIO 18 → 220 Ω
                                                  resistor → LED anode;
                                                  LED cathode → GND

  GND                     Common ground           Shared ground for both
                                                  buttons and both LEDs
  -----------------------------------------------------------------------

### Button configuration

The firmware configures both button pins as `INPUT_PULLUP`.

-   Button released: GPIO reads `HIGH`.
-   Button pressed: GPIO reads `LOW`.
-   Connect one side of each button to its GPIO and the other side to
    GND.
-   No external pull-up resistor is required.

### LED polarity and resistors

The LED long leg is normally the anode (+), and the short leg is
normally the cathode (−). Connect a separate 220-ohm resistor in series
with each LED:

-   GPIO 19 → 220 Ω resistor → Green LED anode; Green LED cathode → GND.
-   GPIO 18 → 220 Ω resistor → Red LED anode; Red LED cathode → GND.

**Important:** The updated pin allocation is: - Product A button: GPIO
25 - Product B button: GPIO 26 - Green LED: GPIO 19 - Red LED: GPIO 18

## 6. Software requirements

-   Arduino IDE 2.x
-   ESP32 board package by Espressif Systems
-   USB data cable and the appropriate USB-to-serial driver for the
    development board

No third-party Arduino library is required by the firmware.

## 7. Build and upload instructions

1.  Install Arduino IDE 2.x.

2.  Open **File → Preferences**.

3.  Add this URL to **Additional Boards Manager URLs**:

    `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

4.  Open **Tools → Board → Boards Manager**.

5.  Search for `esp32` and install **esp32 by Espressif Systems**.

6.  Connect the ESP32 to the computer using a USB data cable.

7.  Open `src/sketch.ino` in Arduino IDE, or paste the firmware into a
    new sketch.

8.  Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module** (or the
    exact board corresponding to your hardware).

9.  Select the correct serial port under **Tools → Port**.

10. Click **Verify** and then **Upload**.

11. Open **Tools → Serial Monitor**, set the baud rate to **115200**,
    and select **Newline** as the line ending.

If upload fails, check the selected port, USB cable, board selection,
and USB-to-serial driver. Some ESP32 boards require holding the BOOT
button while upload begins.

## 8. Operating instructions

Enter one command at a time in the Serial Monitor and press Enter.

  Command       Action
  ------------- ---------------------------------------------------
  `PAY 10`      Add INR 10 to credit
  `PAY 20`      Add INR 20 to credit
  `VEND_OK`     Confirm successful vending while a vend is active
  `VEND_FAIL`   Report a failed vend while a vend is active

Use the physical buttons to select Product A or Product B.
Acknowledgement commands are accepted only while the controller is in
the `VENDING` state.

## 9. State machine

  -----------------------------------------------------------------------
  State                               Meaning
  ----------------------------------- -----------------------------------
  `IDLE`                              No available credit; waiting for
                                      payment

  `CREDIT_AVAILABLE`                  Credit is available; waiting for
                                      product selection

  `VENDING`                           An eligible product was selected;
                                      waiting for acknowledgement

  `SUCCESS`                           Successful acknowledgement
                                      received; product price deducted

  `FAILED`                            Vend failed, timed out, or product
                                      selection was rejected
  -----------------------------------------------------------------------

Typical transaction flow:

``` text
IDLE
  |
  | PAY 10 / PAY 20
  v
CREDIT_AVAILABLE
  |
  | Product button pressed
  | Sufficient credit
  v
VENDING
  |
  +---- VEND_OK ------> SUCCESS
  |                       |
  |                       | Deduct product price
  |                       v
  |                 IDLE / CREDIT_AVAILABLE
  |
  +---- VEND_FAIL ---> FAILED
  |
  +---- 5s timeout ---> FAILED
                          |
                          | Preserve credit
                          v
                    IDLE / CREDIT_AVAILABLE
```

If the selected product costs more than the available credit, the
controller rejects the selection and does not start vending. The
`FAILED` state is displayed briefly before the controller returns to the
appropriate ready state.

## 10. LED indication

  Controller state     Green LED (GPIO 19)                   Red LED (GPIO 18)
  -------------------- ------------------------------------- -------------------
  `IDLE`               OFF                                   OFF
  `CREDIT_AVAILABLE`   ON                                    OFF
  `VENDING`            ON                                    ON
  `SUCCESS`            ON                                    OFF
  `FAILED`             ON if credit remains; otherwise OFF   Blinking

The LEDs provide status indication; detailed transaction information is
printed in the Serial Monitor.

## 11. Test plan

Run these tests after uploading the firmware.

### Test 1 --- Successful Product A transaction

1.  Power on or reset the ESP32.
2.  Enter `PAY 20`.
3.  Press the Product A button on GPIO 25.
4.  Within five seconds, enter `VEND_OK`.

**Expected:** INR 15 is deducted and remaining credit is INR 5.

### Test 2 --- Insufficient credit

1.  Start with zero credit.
2.  Enter `PAY 10`.
3.  Press the Product B button on GPIO 26.

**Expected:** Product B is rejected because it costs INR 25. Credit
remains INR 10 and vending does not start.

### Test 3 --- Vend failure

1.  Enter `PAY 20`.
2.  Press the Product A button on GPIO 25.
3.  Enter `VEND_FAIL` within five seconds.

**Expected:** The transaction enters `FAILED`. Credit remains INR 20.

### Test 4 --- Acknowledgement timeout

1.  Enter `PAY 20`.
2.  Press the Product A button.
3.  Do not send an acknowledgement.
4.  Wait at least five seconds.

**Expected:** The transaction times out and enters `FAILED`. Credit
remains INR 20.

### Test 5 --- Invalid payment commands

Try `PAY -10`, `PAY 15`, `PAY 100`, and `PAY ABC`.

**Expected:** Each unsupported payment command is rejected and the
balance is not increased.

### Test 6 --- Repeated transactions

Complete a successful transaction, add more credit, and perform another
selection without resetting the ESP32.

**Expected:** The controller continues operating and maintains the
remaining credit correctly.

## 12. Implementation notes

-   The button handler uses a 50 ms debounce interval to reduce false
    triggers from mechanical switch bounce.
-   The acknowledgement timeout is checked using `millis()`, not a
    blocking five-second delay. This allows the main loop to continue
    reading serial input and updating LEDs.
-   Product price is deducted only in the successful acknowledgement
    handler.
-   Failure and timeout paths do not deduct credit.
-   Payment is accepted only in `IDLE` or `CREDIT_AVAILABLE`.
-   Acknowledgements received outside `VENDING` are rejected.
-   The firmware reports uptime in seconds; no real-time clock is
    included.

## 13. Recommended repository structure

``` text
Voltnite_ESP32_Vending_Assignment_Hemant_Shakywar/
│
├── README.md
├── src/
│   └── sketch.ino
├── circuit/
│   └── circuit_diagram.png
├── simulation/
│   └── wokwi_project_link.txt
├── documentation/
│   └── state_machine.png
└── demo/
    └── demo_video_link.txt
```

Add the actual circuit diagram, Wokwi project link (if used), and demo
link before submission. Replace the candidate name if required.

## 14. Demo video checklist

A 2--5 minute demonstration should show:

1.  ESP32 startup and initial zero credit.
2.  Payment using `PAY 20`.
3.  Product A selection using the GPIO 25 button.
4.  Successful `VEND_OK` acknowledgement and remaining balance of INR 5.
5.  Insufficient-credit rejection for Product B using the GPIO 26
    button.
6.  A failed vend using `VEND_FAIL`, showing that credit is preserved.
7.  An acknowledgement timeout, showing that credit is preserved.
8.  Invalid payment command rejection.

## 15. Limitations and possible extensions

This implementation uses Serial Monitor acknowledgements to simulate the
vending mechanism. It does not include a physical product dispenser,
real payment gateway, or Wi-Fi dashboard. These may be added as optional
extensions while retaining the core credit-safety logic.

## 16. Submission checklist

-   [ ] Complete firmware source code included.
-   [ ] Updated circuit schematic included, showing the GPIO 25/26
    buttons and GPIO 19/18 LEDs.
-   [ ] README and build/run instructions included.
-   [ ] Wokwi project link included, if simulation is used.
-   [ ] Demo video or demonstration link included.
-   [ ] Firmware compiled and tested on the target ESP32 or in Wokwi.
-   [ ] Test results verified before submission.

------------------------------------------------------------------------

**Project:** Voltnite ESP32 Smart Vending Controller\
**Candidate:** Hemant Shakywar\
**Updated GPIO mapping:** Buttons --- GPIO 25 and 26; LEDs --- GPIO 19
and 18.
