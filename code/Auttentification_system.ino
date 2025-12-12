#include "pitches.h"

#include <string.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 8
MFRC522 mfrc522(SS_PIN, RST_PIN);
#define GREEN_LED A0
#define RED_LED A1
#define BLUE_LED A2
#define BUZZER A3
#define POT 4
#define BUTTON A5
#define SERVO_PIN 9

const int rs = 2,
          en = 3,
          d4 = 4,
          d5 = 5,
          d6 = 6,
          d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Servo myservo;
int position = 0;

int cheer_sound[] = {
  NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5
};
int cheer_sound_duration[] = {
  4, 4, 4, 4, 1
};
int alert_sound[] = {
  NOTE_E5, NOTE_DS5, NOTE_C5, NOTE_G4, NOTE_C4
};
int alert_sound_duration[] = {
  4, 4, 4, 4, 1
};
bool is_auth = false;

String admin_card = "C0 FE EC 32";
String user_cards[] = {
  "00 00 00 00",
  "00 00 00 00",
  "00 00 00 00",
  "00 00 00 00",
  "00 00 00 00"
};

int tasks[10][3] = {
  { 3, 3, 2 },
  { 4, 2, 3 },
  { 1, 5, 3 },
  { 8, 2, 5 },
  { 7, 5, 2 },
  { 2, 9, 1 },
  { 5, 3, 0 },
  { 8, 3, 6 },
  { 9, 5, 6 },
  { 8, 4, 4 }
};
int tasks_size = 10;
int tasks_counter = 0;
int user_cards_size = 5;

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Approximate your card to the reader...");
  Serial.println();
  lcd.print("Entry by cards");
  myservo.attach(SERVO_PIN);
  myservo.write(0);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);
}

void loop() {
  is_auth = false;
  tasks_counter++;
  if (tasks_counter >= tasks_size)
    tasks_counter = 0;
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  for (int i = 0; i < user_cards_size; i++) {
    if (content.substring(1) == user_cards[i]) {
      solve_task();
      is_auth = true;
    }
  }
  if (content.substring(1) == admin_card)
    open_admin_settings();
  if (!is_auth)
    access_denied();
}

void access_denied() {
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  Serial.println("Access denied");
  lcd.clear();
  lcd.print("Access denied!");
  play_alert();
  reset_auth();
}

void access_auth() {
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("Access granted!");
  lcd.clear();
  lcd.print("Access granted!");
  myservo.write(180);
  play_melody();
  reset_auth();
}

void reset_auth() {
  delay(3000);
  myservo.write(0);
  lcd.clear();
  lcd.print("Waiting for key!");
}

void play_alert() {
  for (int thisNote = 0; thisNote < (sizeof(alert_sound) / sizeof(int)); thisNote++) {

    int noteDuration = 1000 / alert_sound_duration[thisNote];
    tone(BUZZER, alert_sound[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}

void play_melody() {
  for (int thisNote = 0; thisNote < (sizeof(cheer_sound) / sizeof(int)); thisNote++) {

    int noteDuration = 1000 / cheer_sound_duration[thisNote];
    tone(BUZZER, cheer_sound[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}

void solve_task() {
  lcd.clear();
  int task_result = tasks[tasks_counter][0] + tasks[tasks_counter][1] * tasks[tasks_counter][2];
  int user_result = 0;
  char task_string[5];
  task_string[0] = '0'+tasks[tasks_counter][0];
  task_string[1] = '+';
  task_string[2] = '0'+tasks[tasks_counter][1];
  task_string[3] = '*';
  task_string[4] = '0'+tasks[tasks_counter][2];
  task_string[5] = '\0';
  int button_state = digitalRead(BUTTON);
  while (button_state == LOW) {
    Serial.print(button_state);
    lcd.clear();
    user_result = analogRead(POT) / 20;
    button_state = digitalRead(BUTTON);
    lcd.print("Solve: ");
    lcd.setCursor(0, 2);
    lcd.print(task_string);
    lcd.print('=');
    lcd.setCursor(6, 2);
    lcd.print(user_result);
    delay(100);
  }
  if (user_result == task_result) {
    access_auth();
  } else {
    access_denied();
  }
}

void open_admin_settings() {
  int operation = 0;
  int button_state = digitalRead(BUTTON);
  while (button_state == LOW) {
    lcd.clear();
    lcd.print("Choose operation: ");
    lcd.setCursor(0, 2);
    lcd.print("0-E 1-A 2-R");
    lcd.setCursor(13, 2);
    operation = analogRead(POT) / 500;
    button_state = digitalRead(BUTTON);
    lcd.print(operation);
    delay(100);
  }
  switch (operation) {
    case 0:
      access_auth();
      is_auth = true;
      return;
    case 1:
      lcd.clear();
      lcd.print("Show card to add");
      delay(3000);
      add_card();
      break;
    case 2:
      lcd.clear();
      lcd.print("Show card to rem");
      delay(3000);
      rem_card();
      break;
    default:
      lcd.clear();
      lcd.print("Invalid operation");
      delay(3000);
      reset_auth();
  }
}

void add_card() {
  lcd.clear();
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  String input_card = content.substring(1);
  content.toUpperCase();
  int i;
  for (i = 0; i < user_cards_size; i++) {
    if (input_card != "00 00 00 00") {
      user_cards[i] = content.substring(1);
      lcd.print("Add is done");
      delay(3000);
      is_auth = true;
      reset_auth();
      return;
    }
    if (input_card == admin_card) {
      open_admin_settings();
      return;
    }
  }
  lcd.clear();
  lcd.print("No enough space");
  delay(3000);
  reset_auth();
}

void rem_card() {
  lcd.clear();
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  String card_to_remove = content.substring(1);
  for (int i = 0; i < user_cards_size; i++) {
    if (user_cards[i] == card_to_remove) {
      user_cards[i] = "00 00 00 00";
      lcd.print("Rem is done");
      delay(3000);
      is_auth = true;
      reset_auth();
      return;
    }
  }
  lcd.clear();
  lcd.print("No such card");
  delay(3000);
  reset_auth();
}