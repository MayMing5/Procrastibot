// include the library code:
#include <LiquidCrystal.h>
#include <Servo.h>

// lcd arduino pin numbers
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int buttonPin = A0;  // the number of the pushbutton pin
Servo lockServo;  // create Servo object to control a servo
// pin number of the sensor's output:
const int trigPin = 7; // outputs signal
const int echoPin = 8;  // receives echoed signal to read in data
const int relayPin1 = 9;
const int relayPin2 = 10;
bool lockState = false;  // variable for door lock status (0 (false) - unlocked, 1 (true) - locked)
int pos = 0;    // variable to store the servo position
unsigned long startTime;
bool timerStarted = false;
bool timerFinished = false;

int distances[5] = {100, 100, 100, 100, 100};
int count = 0;
unsigned long startTimeUS = 0;
bool isClose = false;
bool runningAway = false;
const int close_distance = 2; //inches; the distance we consider to be too close
long duration, inches, cm;

void setup() {
  lockServo.attach(13);  // attaches the servo on pin 9 to the Servo object
  pinMode(buttonPin, INPUT); // initialize the pushbutton pin as an input:
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(13, OUTPUT);
  lcd.begin(16, 2); // set up the LCD's number of columns and rows:
  lcd.print("Press to begin"); // Print a message to the LCD.
}



void loop() {
  double buttonState = analogRead(buttonPin); // read the state of the pushbutton value:
  Serial.println("Button State:");
  Serial.println(buttonState);

  // if not locked & button pushed-> lock door
  if (!lockState && buttonState > 400) {
    lcd.clear();
    lcd.print("Timer: ");
    Serial.println("locking");
    lock();
    delay(300);
    startTime = millis();
    timerStarted = true;
  }
 
  unsigned long elapsedSeconds = (millis() - startTime) / 1000;
  Serial.println("Elapsed seconds: ");
  Serial.println(elapsedSeconds);

  // //unlocking condition
  if (!lockState && elapsedSeconds == 15 && timerFinished) { // if it is locked and five seconds have passed after the timer has finished
    Serial.println("unlocking");
    unlock();
    displayTimeExpired();
    delay(300);
  }
 
 // if its locked display the timer
  if (lockState) {
    int secondsLeft = ((3 * 5) - elapsedSeconds) % 60;
    int minutesLeft = ((3 * 5) - elapsedSeconds) / 60;
    displayTime(minutesLeft, secondsLeft);
    if (secondsLeft <= 0 && minutesLeft <= 0) {
      timerFinished = true;
      lockState = false;  
    }
  }
  // establish variables for duration of the ping, and the distance result
  // in inches and centimeters:
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);

  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  //inches = ultrasonicDistance();

  int avg = 0;
  for (int i = 0; i < 5; i++) {
    avg += distances[i] / 5;
  }
  distances[count % 5] = inches;
  count++;

  if (avg <= close_distance) { // within close_distance
    if (!isClose) { //not false -> true
      isClose = true;
      startTimeUS = millis();  // Start timing (millis returns miliseconds since called)
    } else if ((millis() - startTimeUS >= close_distance*700) && lockState) {
      Serial.println("Distance <= 5 inches for 5 seconds. Running away!");
        // Do something to signify "return 0" – depends on context
        if (isClose && !runningAway) {
          runningAway = true;
          runAway();
          isClose = false;
        }
    }
  } else {
    isClose = false; // Reset if condition fails
    runningAway = false; // Reset trigger after object moves away
  }

  delay(300);
}

void unlock() {
  for (pos = 0; pos <= 90; pos += 5) { // goes from 0 degrees to 180 degrees
    lockServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  lockState = false;
  return;
}


void lock() {
  for (pos = 90; pos >= 0; pos -= 5) { // goes from 180 degrees to 0 degrees
    lockServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  lockState = true;
  return;
}


void displayTime(int minutesLeft, int secondsLeft) {
  lcd.setCursor(0, 1); // set the cursor to column 0, line 1
  lcd.print(minutesLeft ); // print the number of seconds since reset:
  lcd.setCursor(2, 1);
  lcd.print(":");
  if (secondsLeft <= 9) {
    lcd.setCursor(3, 1);
    lcd.print("0");
    lcd.setCursor(4, 1);
    lcd.print(secondsLeft);
  } else {
    lcd.setCursor(3, 1);
    lcd.print(secondsLeft);
  }
}


void displayTimeExpired() {
  lcd.clear();                      // Clear the screen
  lcd.setCursor(0, 0);              // Top line
  lcd.print("Timer complete!");      // Message
  lcd.setCursor(0, 1);              // Bottom line
  lcd.print("Door unlocked :)");       // Optional extra info
}


long microsecondsToInches(long microseconds) {
  // 73.746 microseconds per inch
  // This gives the distance travelled by the ping, outbound and return,
  // so we divide by 2 to get the distance of the obstacle.
  return microseconds / 74 / 2;
}


long microsecondsToCentimeters(long microseconds) {
// 29 microseconds per cm -> take half bc it travels to and from object
  return microseconds / 29 / 2;
}

void runAway() {
  digitalWrite(relayPin1, HIGH);  // Turn relay ON
  digitalWrite(relayPin2, HIGH);  // Turn relay ON
  delay(3000);                   // Wait 3 seconds
  
  digitalWrite(relayPin1, LOW);   // Turn relay OFF
  digitalWrite(relayPin2, LOW);   // Turn relay OFF
  delay(3000);                   // Wait 3 seconds


  runningAway = false;
  return;
}

long ultrasonicDistance() {
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);

  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  return inches;
}