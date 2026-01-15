// Code for the traditional system:

// Pin definitions
#define A_RED 12
#define A_YELLOW 13
#define A_GREEN 3

#define B_RED 2
#define B_YELLOW 9
#define B_GREEN 10

void setup() {
  pinMode(A_RED, OUTPUT);
  pinMode(A_YELLOW, OUTPUT);
  pinMode(A_GREEN, OUTPUT);

  pinMode(B_RED, OUTPUT);
  pinMode(B_YELLOW, OUTPUT);
    pinMode(B_GREEN, OUTPUT);

  // Turn all off initially
  digitalWrite(A_RED, LOW);
  digitalWrite(A_YELLOW, LOW);
  digitalWrite(A_GREEN, LOW);
  digitalWrite(B_RED, LOW);
  digitalWrite(B_YELLOW, LOW);
  digitalWrite(B_GREEN, LOW);

  Serial.begin(9600);
  Serial.println("Traffic light system starting...");
  delay(1000);

  // Start: A Green, B Red
  digitalWrite(A_GREEN, HIGH);
  digitalWrite(B_RED, HIGH);
}

void loop() {
  // === A GREEN, B RED ===
  Serial.println("\nA GREEN phase");
  Serial.println("B RED phase (5 sec wait)");
  countdown("Red", 5);
  delay(5000); // Green duration

  // === A YELLOW ===
  Serial.println("A YELLOW (2 sec)");
  digitalWrite(A_GREEN, LOW);
  digitalWrite(A_YELLOW, HIGH);
  delay(2000);
  digitalWrite(A_YELLOW, LOW);

  // === SWITCH: A RED, B GREEN ===
  Serial.println("\nSwitching sides...");
  digitalWrite(A_RED, HIGH);
  digitalWrite(B_RED, LOW);
  digitalWrite(B_GREEN, HIGH);

  Serial.println("B GREEN phase");
  Serial.println("A RED phase (5 sec wait)");
  countdown("Red", 5);
  delay(5000); // Green duration

  // === B YELLOW ===
  Serial.println("B YELLOW (2 sec)");
  digitalWrite(B_GREEN, LOW);
  digitalWrite(B_YELLOW, HIGH);
  delay(2000);
  digitalWrite(B_YELLOW, LOW);

  // === BACK TO START ===
  digitalWrite(b_RED, HIGH);
  digitalWrite(A_RED, LOW);
  digitalWrite(A_GREEN, HIGH);
}

 Helper function to print countdowns
void countdown(String color, int seconds) {
  for (int i = seconds; i> 0; i--) {
    Serial.print(color);
    Serial.print(" changing in ");
    Serial.println(i);
    delay(1000);
  }
}
