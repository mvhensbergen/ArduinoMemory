// Storage of the sequence
const int maxRounds = 7; // Maximum rounds
int currentLength = 0;
int sequence[maxRounds+1];

int currentGuess = 0;
int guess[maxRounds+1];

#define MODE_SHOWING_SEQUENCE 0
#define MODE_ACQUIRE_INPUT 1
#define MODE_CALCULATE_RESULT 2
#define MODE_RESET 3
#define MODE_WON 4
int mode;  // The current state of the state machine

int shortDelay = 250;  // These values define how fast the red LED should flash
int longDelay = 1000;

int ledPin = 30;
int buzzerPin = 32;
int readyInput = 34;
int buttonPin = 2;  // Pins in the 30 range are OUTPUT only but the button requires INPUT

#define LONG_PRESS_LENGTH 500

#define NOTE_C4  262
#define NOTE_G4  392
#define NOTE_B4  494
#define NOTE_E4  330
#define NOTE_C5  523
#define NOTE_E3  165
#define NOTE_C3  131

// Music for start of game
void playStart() {
  int toneLength = 500;
  tone(buzzerPin, NOTE_E4, 1000/4);
  delay(toneLength);
  tone(buzzerPin, NOTE_E4, 1000/4);
  delay(toneLength);
  tone(buzzerPin, NOTE_E4, 1000/4);
  delay(toneLength);
  tone(buzzerPin, NOTE_B4, 1000/2);
  delay(toneLength);
}

// Music for losing the game
void playFail() {
  tone(buzzerPin, NOTE_E3, 1000/4);
  delay(200);
  tone(buzzerPin, NOTE_C3, 1000/4);
}

// Sound effect confirming success
void playSuccess() {
  tone(buzzerPin, NOTE_C4, 1000/4);
  delay(100);
  tone(buzzerPin, NOTE_E4, 1000/4);
  delay(100);
  tone(buzzerPin, NOTE_G4, 1000/4);
  delay(100);
  tone(buzzerPin, NOTE_C5, 1000/4);
}

// Music for winning the game
void playWon() {
  for (int i=0; i<3; i++) {
    tone(buzzerPin, NOTE_C4, 1000/4);
    delay(100);
    tone(buzzerPin, NOTE_E4, 1000/4);
    delay(100);
    tone(buzzerPin, NOTE_C4, 1000/4);
    delay(100);
    tone(buzzerPin, NOTE_C5, 1000/4);
  }
}

// Checks if the user guessed the proper sequence
bool compare() {
  for ( int i = 0; i < currentLength; i++ ) {
    if (guess[i] != sequence[i]) {
      return false;
    }
  }
  return true;
}

// Add a random item to the sequence, thereby extending it by 1 in length
// 0 = short, 1 = long
void createSequence() {
  sequence[currentLength] = random(0,2);
}

// Play the sequence to be guessed
void showSequence() {
  Serial.println("Play");
  for (int i = 0; i < currentLength; i++ ) {
    Serial.print(sequence[i]);
    digitalWrite(ledPin, HIGH);
    if ( sequence[i] == 0 ) {
      delay(shortDelay);
    } else {  // Long
      delay(longDelay);
    }
    digitalWrite(ledPin, LOW);
    delay(500);
  }
  Serial.println();
}

unsigned long timeButton = 0;

// Reset the state of the game to its starting position
void reset() {
  currentLength = 0;
  createSequence();
  currentLength = 1;

  playStart();
}

// Initialization on first run
void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(readyInput, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  randomSeed(analogRead(0));  // Initialize the random generator

  mode = MODE_RESET; // Initialise the state machine to (re)set the game to its starting position
}

bool isPressed;
bool isLong = false;

void loop() {
  // Should we reset the game? Happens on first startup, when losing the game or winning the game
  if ( mode == MODE_RESET) {
    reset();
    mode = MODE_SHOWING_SEQUENCE; // After reset, immediately show the generated sequence
  }

  // We should extend the sequence and display it to the user
  if ( mode == MODE_SHOWING_SEQUENCE) {
    digitalWrite(readyInput, LOW);
    createSequence();
    showSequence();
  
    // Set mode to user input
    currentGuess = 0;
    digitalWrite(readyInput, HIGH);
    mode = MODE_ACQUIRE_INPUT;
  }

  // User input handler
  if ( mode == MODE_ACQUIRE_INPUT ) {
    int state = digitalRead(buttonPin); // Get the state of the button
    
    if (state == LOW) { // If the button is depressed, turn off the red led
      digitalWrite(ledPin, LOW);
      if (!isPressed) { // If the buttons was not pressed down earlier, then only
        timeButton = millis();  // record last time the button was not pressed
      } else {  // The buttonn was depressed after being pressed
        if ( isLong ) { // Did we register a long press?
          guess[currentGuess] = 1;
        } else {
          guess[currentGuess] = 0;
        }
        currentGuess += 1;
  
        // Log the guess to serial (debugging code)
        Serial.println("Current Guess:");
        for ( int i = 0; i < currentGuess; i++ ) {
          Serial.print(guess[i]);
        }
        Serial.println();
  
        // If the user guessed all items in sequence, switch to checking the result of the input
        if (currentGuess == currentLength) {
          mode = MODE_CALCULATE_RESULT;
        }
        isPressed = false;
        isLong = false;
      }
    } else {
      // The button is depressed
      isPressed = true;

      long result = millis()-timeButton;  // The button was pressed this long

      // Register as long press if it was pressed longer than 500ms pressed
      if (result > LONG_PRESS_LENGTH) {
       isLong = true;
       digitalWrite(ledPin, LOW);
      } else {
        digitalWrite(ledPin, HIGH);
      }
    }
  }

  if ( mode == MODE_CALCULATE_RESULT) {  // Check if we should advance to next level or end the game due to win or loss
    digitalWrite(readyInput, LOW);
    delay(1000);
    if (!compare()) {  // User guessed incorrectly
      playFail();
      delay(1000);
      mode = MODE_RESET;
    } else {
      playSuccess();
    }
    delay(1000);
    if ( mode != MODE_RESET) { // If the user did not fail before the last round
      if ( currentLength == maxRounds ) {  // Check if we just did the last round
        mode = MODE_WON;  // Game is over and won
      } else {
        mode = MODE_SHOWING_SEQUENCE;  // Continue gaming with the next round!
        currentLength += 1;
      }
    }
  }

  // The user won, so play glorious music and start over
  if ( mode == MODE_WON ) {
    playWon();
    delay(1000);
    mode = MODE_RESET;
  }
}
