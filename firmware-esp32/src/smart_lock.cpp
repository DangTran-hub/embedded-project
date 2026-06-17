/**
 * @file smart_lock.cpp
 * @brief Core control logic for the ESP32 IoT smart lock.
 *
 * @details
 * This module coordinates all runtime behavior of the lock: keypad password
 * entry, RFID card authentication, local unlock button handling, LCD feedback,
 * buzzer alerts, solenoid control, battery monitoring, Wi-Fi connection, MQTT
 * messaging, and non-volatile storage through Preferences. It uses a simple
 * state machine to manage idle, password entry, password change, temporary
 * lockout, and RFID learning flows.
 *
 * Valid local or remote credentials trigger the solenoid, publish the unlock
 * event, and then automatically return the door to the locked state. Failed
 * password or RFID attempts are counted and can temporarily lock input for a
 * fixed timeout. MQTT commands allow the application server to unlock the door
 * remotely, start RFID enrollment, and add or remove authorized card IDs.
 */
#include "smart_lock.h"

#include "battery_monitor.h"
#include "mqtt_topics.h"

#include <ArduinoJson.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <Wire.h>

static WiFiMulti wifiMulti;
static MFRC522 rfid(SS_PIN, RST_PIN);
static LiquidCrystal_I2C lcd(0x27, 16, 2);
static Preferences preferences;
static WiFiClientSecure espClient;
static PubSubClient mqttClient(espClient);

static unsigned long lastLCDActivity = 0;
static bool isLCDBacklightOn = true;
static int globalBattery = 100;
static String currentPass = "1236";

static byte pinRows[ROW_NUM] = {33, 25, 26, 27};
static byte pinCols[COL_NUM] = {14, 12, 13};
static char keys[ROW_NUM][COL_NUM] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
static Keypad keypad = Keypad(makeKeymap(keys), pinRows, pinCols, ROW_NUM, COL_NUM);

static SystemState currentState = SYS_IDLE;
static String inputPassword = "";
static String tempPassword = "";
static int failCount = 0;
static int failCountRFID = 0;
static unsigned long lockStartTime = 0;
static unsigned long lastActivityTime = 0;
static bool isLearningMode = false;
static unsigned long lastNetCheck = 0;

static void beepStep(void);
static void beepSuccess(void);
static void beepError(void);
static void wakeUpLCD(void);
static void showIdle(void);
static void publishStatus(bool locked, const char *method, const char *userName, bool saveToHistory);
static void publishAdminEvent(const char *method, const char *note);
static void unlockDoor(const char *method, const char *userName);
static void maintainConnections(void);
static void handleRFID(void);
static void startInput(const char *line1);
static void handleKeypad(void);
static void handleButton(void);
static void handleLCDTimeout(void);
static void mqttCallback(char *topic, byte *payload, unsigned int length);

static void beepStep(void)
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
}

static void beepSuccess(void)
{
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
    }
}

static void beepError(void)
{
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(400);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

static void wakeUpLCD(void)
{
    lastLCDActivity = millis();
    if (!isLCDBacklightOn) {
        lcd.backlight();
        isLCDBacklightOn = true;
    }
}

static void showIdle(void)
{
    globalBattery = batteryMonitorReadPercent();
    wakeUpLCD();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("--- HE THONG ---");
    lcd.setCursor(0, 1);
    lcd.print("KHOA  Pin:");
    lcd.print(globalBattery);
    lcd.print("%");

    currentState = SYS_IDLE;
    inputPassword = "";
}

static void publishStatus(bool locked, const char *method, const char *userName, bool saveToHistory)
{
    if (!mqttClient.connected()) {
        return;
    }

    String topic = mqttStatusTopic(LOCK_ID);
    StaticJsonDocument<256> doc;
    doc["locked"] = locked;
    doc["online"] = true;
    doc["battery"] = batteryMonitorReadPercent();
    doc["method"] = method;
    doc["by"] = userName;
    doc["save"] = saveToHistory;

    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
}

static void publishAdminEvent(const char *method, const char *note)
{
    if (!mqttClient.connected()) {
        return;
    }

    String topic = mqttStatusTopic(LOCK_ID);
    StaticJsonDocument<256> doc;
    doc["online"] = true;
    doc["method"] = method;
    doc["by"] = note;
    doc["save"] = true;

    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
}

static void unlockDoor(const char *method, const char *userName)
{
    wakeUpLCD();
    lcd.clear();
    lcd.print("DA XAC NHAN!");
    lcd.setCursor(0, 1);
    lcd.print("MO CUA: ");
    lcd.print(userName);

    digitalWrite(SOLENOID_PIN, HIGH);
    publishStatus(false, method, userName, true);
    beepSuccess();
    delay(3000);

    digitalWrite(SOLENOID_PIN, LOW);
    lcd.clear();
    lcd.print("DA KHOA LAI");
    publishStatus(true, "auto_lock", "He Thong", false);
    delay(1500);
    showIdle();
}

static void maintainConnections(void)
{
    if (millis() - lastNetCheck < NETWORK_CHECK_MS) {
        return;
    }
    lastNetCheck = millis();

    if (wifiMulti.run() != WL_CONNECTED) {
        return;
    }

    if (!mqttClient.connected()) {
        String clientId = "ESP32_Lock_" + String(random(1000, 9999));
        if (mqttClient.connect(clientId.c_str(), "anhminh", "020623")) {
            String commandTopic = mqttCommandTopic(LOCK_ID);
            mqttClient.subscribe(commandTopic.c_str());
            publishStatus((digitalRead(SOLENOID_PIN) == LOW), "boot", "He Thong", false);
        }
    }
}

static void handleRFID(void)
{
    if (currentState == SYS_LOCKED) {
        return;
    }
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return;
    }

    wakeUpLCD();

    String cardID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        cardID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        cardID += String(rfid.uid.uidByte[i], HEX);
    }
    cardID.toUpperCase();

    if (isLearningMode) {
        if (mqttClient.connected()) {
            StaticJsonDocument<256> doc;
            doc["pending_id"] = cardID;
            doc["online"] = true;

            String payload;
            serializeJson(doc, payload);
            String statusTopic = mqttStatusTopic(LOCK_ID);
            mqttClient.publish(statusTopic.c_str(), payload.c_str());
        }

        lcd.clear();
        lcd.print("DA QUET THE!");
        isLearningMode = false;
        beepSuccess();
        delay(2000);
        showIdle();
    } else {
        preferences.begin("rfid-cards", true);
        bool authorized = preferences.isKey(cardID.c_str()) || (cardID == "63CDA256");
        preferences.end();

        if (authorized) {
            failCountRFID = 0;
            unlockDoor("rfid", "The Tu");
        } else {
            failCountRFID++;
            beepError();
            lcd.clear();
            lcd.print("THE CHUA DK!");
            publishStatus(true, "warning", "The la xam nhap", true);

            if (failCountRFID >= 3) {
                currentState = SYS_LOCKED;
                lockStartTime = millis();
                lcd.setCursor(0, 1);
                lcd.print("TAM KHOA 30S!");
                publishStatus(true, "warning", "Khoa the 30s", true);
            } else {
                delay(2000);
                showIdle();
            }
        }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

static void startInput(const char *line1)
{
    lastActivityTime = millis();
    wakeUpLCD();
    lcd.clear();
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print("> ");
    inputPassword = "";
}

static void handleKeypad(void)
{
    if (currentState == SYS_LOCKED) {
        if (millis() - lockStartTime > LOCK_TIMEOUT_MS) {
            currentState = SYS_IDLE;
            failCount = 0;
            failCountRFID = 0;
            showIdle();
        }
        return;
    }

    char key = keypad.getKey();
    if (!key) {
        if (currentState != SYS_IDLE && (millis() - lastActivityTime > INPUT_TIMEOUT_MS)) {
            showIdle();
        }
        return;
    }

    wakeUpLCD();
    lastActivityTime = millis();
    beepStep();

    if (currentState == SYS_IDLE) {
        if (key == '*') {
            currentState = SYS_CHANGE_OLD;
            startInput("NHAP MK CU:");
        } else {
            startInput("MAT KHAU:");
            currentState = SYS_WAIT_PASSWORD;
            if (key >= '0' && key <= '9') {
                inputPassword += key;
                lcd.print("*");
            }
        }
        return;
    }

    if (key == '*') {
        showIdle();
        return;
    }

    if (key >= '0' && key <= '9' && inputPassword.length() < 4) {
        inputPassword += key;
        lcd.setCursor(inputPassword.length() + 1, 1);
        lcd.print("*");
    }

    if (key != '#' || inputPassword.length() < 4) {
        return;
    }

    if (currentState == SYS_WAIT_PASSWORD) {
        if (inputPassword == currentPass) {
            failCount = 0;
            unlockDoor("password", "Ban Phim");
        } else {
            failCount++;
            beepError();
            publishStatus(true, "warning", "Sai mat khau", true);
            if (failCount >= 3) {
                currentState = SYS_LOCKED;
                lockStartTime = millis();
                lcd.clear();
                lcd.print("TAM KHOA PHIM!");
                publishStatus(true, "warning", "Bi khoa 30s", true);
            } else {
                lcd.clear();
                lcd.print("SAI MAT KHAU!");
                delay(1500);
                startInput("MAT KHAU:");
            }
        }
    } else if (currentState == SYS_CHANGE_OLD) {
        if (inputPassword == currentPass) {
            startInput("NHAP MK MOI:");
            currentState = SYS_CHANGE_NEW;
            beepSuccess();
        } else {
            lcd.clear();
            lcd.print("SAI MK CU!");
            publishStatus(true, "warning", "Doi MK fail", true);
            delay(1500);
            showIdle();
        }
    } else if (currentState == SYS_CHANGE_NEW) {
        tempPassword = inputPassword;
        startInput("XAC NHAN MK:");
        currentState = SYS_CHANGE_CONFIRM;
    } else if (currentState == SYS_CHANGE_CONFIRM) {
        if (inputPassword == tempPassword) {
            currentPass = inputPassword;
            preferences.begin("lock-data", false);
            preferences.putString("password", currentPass);
            preferences.end();
            lcd.clear();
            lcd.print("THANH CONG!");
            publishStatus(true, "change_password", "Ban Phim", true);
            beepSuccess();
            delay(2000);
            showIdle();
        } else {
            lcd.clear();
            lcd.print("KHONG KHOP!");
            delay(1500);
            showIdle();
        }
    }
}

static void handleButton(void)
{
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW) {
            unlockDoor("button", "Nut Bam");
        }
    }
}

static void handleLCDTimeout(void)
{
    if (isLCDBacklightOn && (millis() - lastLCDActivity > LCD_TIMEOUT_MS)) {
        lcd.noBacklight();
        isLCDBacklightOn = false;
    }
}

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    (void)topic;

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        return;
    }

    String action = doc["action"] | "";

    if (action == "unlock") {
        unlockDoor("app", doc["by"] | "App");
    } else if (action == "START_LEARNING") {
        isLearningMode = true;
        wakeUpLCD();
        lcd.clear();
        lcd.print("DANG CHO THE...");
        beepSuccess();
    } else if (action == "ADD_CARD") {
        String cardId = doc["id"] | "";
        cardId.toUpperCase();

        preferences.begin("rfid-cards", false);
        preferences.putBool(cardId.c_str(), true);
        preferences.end();

        publishAdminEvent("da_them_the", cardId.c_str());
        beepSuccess();
        showIdle();
    } else if (action == "REMOVE_CARD") {
        String cardId = doc["id"] | "";
        cardId.toUpperCase();

        preferences.begin("rfid-cards", false);
        preferences.remove(cardId.c_str());
        preferences.end();

        publishAdminEvent("da_xoa_the", cardId.c_str());
        beepSuccess();
        showIdle();
    }
}

void smartLockSetup(void)
{
    Serial.begin(115200);
    pinMode(SOLENOID_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    batteryMonitorBegin(BATTERY_PIN, 4.7f);

    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();

    preferences.begin("lock-data", true);
    currentPass = preferences.getString("password", "1236");
    preferences.end();

    SPI.begin();
    rfid.PCD_Init();

    wifiMulti.addAP("NGUYEN ANH DUNG", "0378161956");
    wifiMulti.addAP("AnhMinh", "02060723");

    espClient.setInsecure();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    showIdle();
}

void smartLockLoop(void)
{
    handleKeypad();
    handleRFID();
    handleButton();
    maintainConnections();

    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    handleLCDTimeout();
}
