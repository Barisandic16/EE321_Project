#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Servo.h>


LiquidCrystal lcd(7, 6, 5, 4, 3, 2);


const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {8, 9, 10, 11};
byte colPins[COLS] = {12, 13, A0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


Servo doorServoA;
Servo doorServoB;
const int servoPinA = 0;
const int servoPinB = 1;
const int emergencyLedPin = A1; 
const int successLedPin = A2;   


String masterPassword = "1111";
String sidePassword1 = "2222";
String sidePassword2 = "3333";
String baitPassword = "4444";


String inputPassword = "";
char selectedOption = '0';
bool inMainMenu = true;
int successfulAttempts = 0;
int failedAttempts = 0;
bool changingPassword = false;
bool masterCheckForChange = false;
bool awaitingPasswordSelection = false;
char passwordToChange = '0';
bool masterLock = false;
bool timeLock = false;
unsigned long timeLockStart = 0;
const int lockDuration = 20000;


void openSection(Servo &servo) {
  servo.write(0);    // Açma pozisyonu
  delay(800);
  servo.write(90);   // Başlangıca geri
  delay(500);
}

void setup() {
  lcd.begin(16, 2);
  pinMode(emergencyLedPin, OUTPUT);
  pinMode(successLedPin, OUTPUT);
  digitalWrite(emergencyLedPin, LOW);
  digitalWrite(successLedPin, LOW);
  doorServoA.attach(servoPinA);
  doorServoB.attach(servoPinB);
  doorServoA.write(90);
  doorServoB.write(90);
  showMainMenu();
}

void loop() {
  char key = keypad.getKey();

  if (timeLock) {
    unsigned long currentMillis = millis();
    if (currentMillis - timeLockStart >= lockDuration) {
      timeLock = false;
      lcd.clear();
      showMasterLock();
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Wait ");
      lcd.print((lockDuration - (currentMillis - timeLockStart)) / 1000);
      lcd.print("s ");
    }
    return;
  }

  if (key) {
    if (masterLock) {
      processMasterLock(key);
    }
    else if (inMainMenu) {
      if (key >= '1' && key <= '4') {
        selectedOption = key;
        inMainMenu = false;
        processSelection(selectedOption);
      }
    }
    else {
      if (masterCheckForChange || awaitingPasswordSelection || changingPassword) {
        processPasswordChange(key);
      } else {
        processInput(key);
      }
    }
  }
}

void showMainMenu() {
  lcd.clear();
  lcd.print("Main Menu:");
  lcd.setCursor(0,1);
  lcd.print("1A 2B 3CP 4RP");
}

void processSelection(char option) {
  lcd.clear();
  inputPassword = "";

  if (option == '1') {
    lcd.print("Enter Pass A:");
  } else if (option == '2') {
    lcd.print("Enter Pass B:");
  } else if (option == '3') {
    lcd.print("Enter Master P:");
    masterCheckForChange = true;
  } else if (option == '4') {
    lcd.clear();
    lcd.print(successfulAttempts);
    lcd.print("S ");
    lcd.print(failedAttempts);
    lcd.print("F");
    delay(3000);
    returnMainMenu();
  }
}

void processInput(char key) {
  if (key == '*') {
    inputPassword = "";
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
  } else if (key == '#') {
    checkPassword();
  } else {
    inputPassword += key;
    lcd.setCursor(inputPassword.length() - 1, 1);
    lcd.print('*');
  }
}

void checkPassword() {
  if (selectedOption == '1') {
    handleSection(doorServoA, "A");
  }
  else if (selectedOption == '2') {
    handleSection(doorServoB, "B");
  }
}

void handleSection(Servo &servo, String sectionName) {
  if (inputPassword == masterPassword || (sectionName == "A" && inputPassword == sidePassword1) || (sectionName == "B" && inputPassword == sidePassword2)) {
    lcd.clear();
    lcd.print("Opening " + sectionName + "...");
    successfulAttempts++;
    digitalWrite(successLedPin, HIGH);   
    openSection(servo);
    delay(1000);
    digitalWrite(successLedPin, LOW);    
  }
  else if (inputPassword == baitPassword) {
    lcd.clear();
    lcd.print("Opening " + sectionName + "...");
    openSection(servo);
    blinkEmergencyLed(); 
  }
  else {
    lcd.clear();
    lcd.print("Wrong Password");
    failedAttempts++;
    digitalWrite(emergencyLedPin, HIGH); 
    delay(2000);
    digitalWrite(emergencyLedPin, LOW);
    attemptCounter();
  }
  returnMainMenu();
}

void blinkEmergencyLed() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(emergencyLedPin, HIGH);
    delay(200);
    digitalWrite(emergencyLedPin, LOW);
    delay(200);
  }
}

void processPasswordChange(char key) {
  if (key == '*') {
    inputPassword = "";
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
  }
  else if (key == '#') {
    if (masterCheckForChange) {
      if (inputPassword == masterPassword) {
        lcd.clear();
        lcd.print("1M 2A 3B 4T");
        masterCheckForChange = false;
        awaitingPasswordSelection = true;
      }
      else {
        lcd.clear();
        lcd.print("Wrong Master");
        delay(2000);
        returnMainMenu();
      }
      inputPassword = "";
    }
    else if (changingPassword) {
      if (passwordToChange != '0') {
        if (passwordToChange == '1') masterPassword = inputPassword;
        else if (passwordToChange == '2') sidePassword1 = inputPassword;
        else if (passwordToChange == '3') sidePassword2 = inputPassword;
        else if (passwordToChange == '4') baitPassword = inputPassword;

        lcd.clear();
        lcd.print("Password Changed");
        delay(2000);
        returnMainMenu();
      }
    }
  }
  else {
    if (masterCheckForChange) {
      inputPassword += key;
      lcd.setCursor(inputPassword.length() - 1, 1);
      lcd.print('*');
    }
    else if (awaitingPasswordSelection) {
      if (key >= '1' && key <= '4') {
        passwordToChange = key;
        changingPassword = true;
        awaitingPasswordSelection = false;
        lcd.clear();
        lcd.print("New Pass:");
        inputPassword = "";
      }
    }
    else if (changingPassword) {
      inputPassword += key;
      lcd.setCursor(inputPassword.length() - 1, 1);
      lcd.print('*');
    }
  }
}

void returnMainMenu() {
  inputPassword = "";
  selectedOption = '0';
  inMainMenu = true;
  changingPassword = false;
  masterCheckForChange = false;
  awaitingPasswordSelection = false;
  passwordToChange = '0';
  showMainMenu();
}

void attemptCounter() {
  if (failedAttempts % 3 == 0) {
    masterLock = true;
    showMasterLock();
  }
}

void showMasterLock() {
  lcd.clear();
  lcd.print("System Locked");
  digitalWrite(emergencyLedPin, HIGH);
}

void processMasterLock(char key) {
  if (key == '*') {
    inputPassword = "";
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
  }
  else if (key == '#') {
    if (inputPassword == masterPassword) {
      masterLock = false;
      digitalWrite(emergencyLedPin, LOW); 
      lcd.clear();
      lcd.print("Unlocked!");
      delay(2000);
      returnMainMenu();
    }
    else {
      lcd.clear();
      lcd.print("Time Lock 20s");
      timeLockStart = millis();
      timeLock = true;
    }
    inputPassword = "";
  }
  else {
    inputPassword += key;
    lcd.setCursor(inputPassword.length() - 1, 1);
    lcd.print('*');
  }
}
