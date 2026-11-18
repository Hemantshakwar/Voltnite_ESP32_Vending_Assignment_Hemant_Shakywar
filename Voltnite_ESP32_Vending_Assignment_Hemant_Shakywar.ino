
/*
  VOLTNITE ENGINEERING CHALLENGE
  ESP32 SMART VENDING CONTROLLER

  Board: ESP32 DevKit V1
  Baud Rate: 115200

  Product A = Rs.15
  Product B = Rs.25

  Payment:
  PAY 10
  PAY 20

  Acknowledgement:
  VEND_OK
  VEND_FAIL
*/

#include <Arduino.h>

// ---------------- PIN CONFIGURATION ----------------

#define BUTTON_A 25
#define BUTTON_B 26

#define GREEN_LED 19
#define RED_LED   18

// ---------------- PRODUCT CONFIGURATION ------------

const int PRICE_A = 15;
const int PRICE_B = 25;

// ---------------- FSM STATES -----------------------

enum VendingState {
  IDLE,
  CREDIT_AVAILABLE,
  VENDING,
  SUCCESS,
  FAILED
};

VendingState currentState = IDLE;

String selectedProduct = "NONE";

long credit = 0;

unsigned long vendStartTime = 0;
unsigned long stateStartTime = 0;

const unsigned long ACK_TIMEOUT = 5000;
const unsigned long RESULT_DISPLAY_TIME = 1200;

// ---------------- BUTTON DEBOUNCE ------------------

class DebouncedButton {

  private:
    int pin;
    int stableState;
    int lastReading;
    unsigned long lastChangeTime;

    const unsigned long debounceDelay = 50;

  public:

    DebouncedButton(int p) {
      pin = p;
      stableState = HIGH;
      lastReading = HIGH;
      lastChangeTime = 0;
    }

    void begin() {
      pinMode(pin, INPUT_PULLUP);
      stableState = digitalRead(pin);
      lastReading = stableState;
    }

    bool pressed() {

      int reading = digitalRead(pin);

      if (reading != lastReading) {
        lastChangeTime = millis();
        lastReading = reading;
      }

      if ((millis() - lastChangeTime) >= debounceDelay) {

        if (reading != stableState) {

          stableState = reading;

          if (stableState == LOW) {
            return true;
          }
        }
      }

      return false;
    }
};

DebouncedButton buttonA(BUTTON_A);
DebouncedButton buttonB(BUTTON_B);

// ---------------- STATE NAME -----------------------

const char* getStateName(VendingState state) {

  switch (state) {

    case IDLE:
      return "IDLE";

    case CREDIT_AVAILABLE:
      return "CREDIT_AVAILABLE";

    case VENDING:
      return "VENDING";

    case SUCCESS:
      return "SUCCESS";

    case FAILED:
      return "FAILED";
  }

  return "UNKNOWN";
}

// ---------------- SERIAL LOGGING -------------------

void printStatus() {

  Serial.println();
  Serial.println("========== VENDING STATUS ==========");

  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");

  Serial.print("Credit: Rs.");
  Serial.println(credit);

  Serial.print("Selected Product: ");
  Serial.println(selectedProduct);

  Serial.print("Current State: ");
  Serial.println(getStateName(currentState));

  Serial.println("====================================");
}

void changeState(VendingState newState) {

  if (currentState != newState) {

    Serial.print("[");
    Serial.print(millis() / 1000);
    Serial.print("s] STATE: ");

    Serial.print(getStateName(currentState));
    Serial.print(" -> ");

    Serial.println(getStateName(newState));

    currentState = newState;
    stateStartTime = millis();
  }
}

// ---------------- LED CONTROL ----------------------

void updateLEDs() {

  switch (currentState) {

    case IDLE:
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      break;

    case CREDIT_AVAILABLE:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;

    case VENDING:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      break;

    case SUCCESS:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;

    case FAILED:

      digitalWrite(GREEN_LED, credit > 0 ? HIGH : LOW);

      // Blink red LED without blocking the program
      digitalWrite(RED_LED,
        ((millis() / 200) % 2) ? HIGH : LOW);

      break;
  }
}

// ---------------- CREDIT MANAGEMENT ----------------

void addCredit(int amount) {

  if (currentState != IDLE &&
      currentState != CREDIT_AVAILABLE) {

    Serial.println("ERROR: Payment not accepted in current state.");
    return;
  }

  credit += amount;

  Serial.print("PAYMENT SUCCESSFUL: Rs.");
  Serial.println(amount);

  Serial.print("AVAILABLE CREDIT: Rs.");
  Serial.println(credit);

  if (credit > 0) {
    changeState(CREDIT_AVAILABLE);
  }

  printStatus();
}

// ---------------- PRODUCT SELECTION ----------------

void selectProduct(String product, int price) {

  if (currentState != IDLE &&
      currentState != CREDIT_AVAILABLE) {

    Serial.println("ERROR: Product selection not allowed.");
    return;
  }

  selectedProduct = product;

  Serial.println();
  Serial.print("Selected: ");
  Serial.println(product);

  Serial.print("Product Price: Rs.");
  Serial.println(price);

  Serial.print("Available Credit: Rs.");
  Serial.println(credit);

  // Check sufficient credit

  if (credit < price) {

    Serial.println("ERROR: INSUFFICIENT CREDIT!");

    Serial.println("Transaction rejected.");
    Serial.println("Credit remains unchanged.");

    changeState(FAILED);
    return;
  }

  // Start vending

  Serial.println("Starting vending operation...");

  vendStartTime = millis();

  changeState(VENDING);

  Serial.println("--------------------------------");
  Serial.println("WAITING FOR ACKNOWLEDGEMENT");
  Serial.println("Type VEND_OK or VEND_FAIL");
  Serial.println("Timeout: 5 seconds");
  Serial.println("--------------------------------");
}

// ---------------- VEND SUCCESS ---------------------

void vendSuccess() {

  int price = 0;

  if (selectedProduct == "PRODUCT A") {
    price = PRICE_A;
  }
  else if (selectedProduct == "PRODUCT B") {
    price = PRICE_B;
  }
  else {
    Serial.println("ERROR: Invalid selected product.");
    changeState(FAILED);
    return;
  }

  // Deduct credit ONLY after successful ACK

  credit -= price;

  Serial.println();
  Serial.println("******** VENDING SUCCESS ********");

  Serial.print("Product: ");
  Serial.println(selectedProduct);

  Serial.print("Deducted: Rs.");
  Serial.println(price);

  Serial.print("Remaining Credit: Rs.");
  Serial.println(credit);

  changeState(SUCCESS);

  printStatus();
}

// ---------------- VEND FAILURE ---------------------

void vendFailure(const char* reason) {

  Serial.println();
  Serial.println("******** VENDING FAILED ********");

  Serial.print("Reason: ");
  Serial.println(reason);

  Serial.println("Credit NOT deducted.");

  Serial.print("Current Credit: Rs.");
  Serial.println(credit);

  changeState(FAILED);

  printStatus();
}

// ---------------- SERIAL COMMAND HANDLER -----------

String serialBuffer = "";

void processCommand(String command) {

  command.trim();
  command.toUpperCase();

  if (command.length() == 0) {
    return;
  }

  Serial.print("COMMAND RECEIVED: ");
  Serial.println(command);

  // ACK commands are only valid during vending

  if (command == "VEND_OK") {

    if (currentState == VENDING) {
      vendSuccess();
    }
    else {
      Serial.println("ERROR: No active vending operation.");
    }

    return;
  }

  if (command == "VEND_FAIL") {

    if (currentState == VENDING) {
      vendFailure("Failure acknowledgement received.");
    }
    else {
      Serial.println("ERROR: No active vending operation.");
    }

    return;
  }

  // Payment commands

  if (command == "PAY 10") {
    addCredit(10);
    return;
  }

  if (command == "PAY 20") {
    addCredit(20);
    return;
  }

  // Reject invalid and negative commands

  if (command.startsWith("PAY")) {

    Serial.println("ERROR: Invalid payment command.");
    Serial.println("Accepted commands: PAY 10 / PAY 20");
    return;
  }

  Serial.println("ERROR: Unknown command.");
}

// ---------------- SERIAL INPUT ---------------------

void readSerialCommands() {

  while (Serial.available()) {

    char c = Serial.read();

    if (c == '\n' || c == '\r') {

      if (serialBuffer.length() > 0) {

        processCommand(serialBuffer);

        serialBuffer = "";
      }
    }
    else if (serialBuffer.length() < 64) {

      serialBuffer += c;
    }
    else {

      Serial.println("ERROR: Command too long.");
      serialBuffer = "";
    }
  }
}

// ---------------- FSM RESULT HANDLING --------------

void handleStateTimeouts() {

  // ACK timeout

  if (currentState == VENDING) {

    if (millis() - vendStartTime >= ACK_TIMEOUT) {

      vendFailure("ACK TIMEOUT - 5 seconds expired.");
    }
  }

  // Return to ready state after showing result

  if (currentState == SUCCESS ||
      currentState == FAILED) {

    if (millis() - stateStartTime >= RESULT_DISPLAY_TIME) {

      selectedProduct = "NONE";

      if (credit > 0) {
        changeState(CREDIT_AVAILABLE);
      }
      else {
        changeState(IDLE);
      }

      printStatus();
    }
  }
}

// ---------------- SETUP -----------------------------

void setup() {

  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  buttonA.begin();
  buttonB.begin();

  Serial.println();
  Serial.println("==================================");
  Serial.println(" VOLTNITE SMART VENDING CONTROLLER");
  Serial.println("==================================");

  Serial.println("ESP32 INITIALIZED");
  Serial.println("Initial Credit: Rs.0");

  Serial.println();
  Serial.println("Available Commands:");
  Serial.println("PAY 10");
  Serial.println("PAY 20");
  Serial.println("VEND_OK");
  Serial.println("VEND_FAIL");

  Serial.println();
  Serial.println("Product A = Rs.15");
  Serial.println("Product B = Rs.25");

  changeState(IDLE);

  printStatus();
}

// ---------------- MAIN LOOP -------------------------

void loop() {

  // Always read serial input

  readSerialCommands();

  // Check ACK timeout and FSM result states

  handleStateTimeouts();

  // Update LED indications

  updateLEDs();

  // Read buttons only in ready states

  if (currentState == IDLE ||
      currentState == CREDIT_AVAILABLE) {

    if (buttonA.pressed()) {
      selectProduct("PRODUCT A", PRICE_A);
    }

    if (buttonB.pressed()) {
      selectProduct("PRODUCT B", PRICE_B);
    }
  }

  // No blocking delay here
}
