#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 

// Create display object (I2C Address: 0x3C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin definitions
#define SS 5
#define RST 14
#define DIO0 2
#define BUTTON_NEXT 25
#define BUTTON_PREV 26
#define BUTTON_SELC 33
#define BUTTON_BACK 32

// Device ID (should be unique)
const char* ID = "AB2";

// Menus definition
const char* mainMenu[] = { "Send Message", "Received Message" };
const int mainMenuSize = 2;

const char* sendMenu[] = { "Stranded", "Injury", "Fire", "Hostiles" };
const int sendMenuSize = 4;

// For menu navigation state
enum MenuState { MAIN_MENU, SEND_MENU, RECEIVE_MENU };
MenuState currentMenu = MAIN_MENU;
int currentSelection = 0;  // used for whichever menu is active

// Structure and storage for received messages (last 3)
struct ReceivedMessage {
  String senderID;
  String message;
};
ReceivedMessage receivedMessages[3];
int receivedCount = 0;  // counts all messages received

// ------------------------
// Utility Functions
// ------------------------

// Store a new received message in a circular buffer of size 3.
void storeReceivedMessage(String sender, String msg) {
  int index = receivedCount % 3;
  receivedMessages[index].senderID = sender;
  receivedMessages[index].message = msg;
  receivedCount++;
}

// ------------------------
// Menu Display Functions
// ------------------------

// Display the main menu
void displayMainMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Main Menu:");
  for (int i = 0; i < mainMenuSize; i++) {
    if (i == currentSelection)
      display.print("> ");
    else
      display.print("  ");
    display.println(mainMenu[i]);
  }
  display.display();
}

// Display the send message menu
void displaySendMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Send Menu:");
  for (int i = 0; i < sendMenuSize; i++) {
    if (i == currentSelection)
      display.print("> ");
    else
      display.print("  ");
    display.println(sendMenu[i]);
  }
  display.display();
}

// Display the received messages menu
void displayReceiveMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Received Msgs:");
  // Determine how many messages to show (up to 3)
  int count = min(receivedCount, 3);
  // Show messages in reverse order (newest first)
  for (int i = 0; i < count; i++) {
    // Calculate index from the circular buffer (newest at index (receivedCount-1)%3)
    int index = (receivedCount - 1 - i) % 3;
    display.print(receivedMessages[index].senderID);
    display.print(": ");
    display.println(receivedMessages[index].message);
    display.println("----------------");
  }
  display.display();
}

// ------------------------
// Message Sending Function
// ------------------------

// Send the currently selected message from the send menu
void sendSelectedMessage() {
  String message = sendMenu[currentSelection];
  // Prepend our device ID so receivers know who sent it.
  String fullMessage = String(ID) + ": " + message;
  
  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(fullMessage);
  LoRa.endPacket();
  
  // Provide user feedback
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Message Sent:");
  display.println(fullMessage);
  display.display();
  delay(2000);  // brief pause for user to read
}

// ------------------------
// LoRa Receive Function
// ------------------------

// Check for incoming LoRa messages and store them
void checkForReceivedMessage() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    // Assume format "SenderID: message"
    int sep = received.indexOf(':');
    String sender = "Unknown";
    String msgText = received;
    if (sep != -1) {
      sender = received.substring(0, sep);
      msgText = received.substring(sep + 2);  // skip the colon and space
    }
    storeReceivedMessage(sender, msgText);
    
    // Show a brief notification
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Msg Received:");
    display.println(received);
    display.display();
    delay(2000);
    
    // Refresh current menu display after notification
    if (currentMenu == RECEIVE_MENU)
      displayReceiveMenu();
    else if (currentMenu == MAIN_MENU)
      displayMainMenu();
    else if (currentMenu == SEND_MENU)
      displaySendMenu();
  }
}

// ------------------------
// Menu Navigation Handler
// ------------------------

// Reads button states (active LOW) and updates the current menu/selection.
void updateMenu() {
  int btn_next = digitalRead(BUTTON_NEXT);
  int btn_prev = digitalRead(BUTTON_PREV);
  int btn_selc = digitalRead(BUTTON_SELC);
  int btn_back = digitalRead(BUTTON_BACK);
  
  // Navigate "up" in the current menu
  if (btn_next == LOW) {
    if (currentMenu == MAIN_MENU) {
      currentSelection = (currentSelection - 1 + mainMenuSize) % mainMenuSize;
      displayMainMenu();
    } else if (currentMenu == SEND_MENU) {
      currentSelection = (currentSelection - 1 + sendMenuSize) % sendMenuSize;
      displaySendMenu();
    }
    delay(300);  // debounce
  }
  
  // Navigate "down" in the current menu
  if (btn_prev == LOW) {
    if (currentMenu == MAIN_MENU) {
      currentSelection = (currentSelection + 1) % mainMenuSize;
      displayMainMenu();
    } else if (currentMenu == SEND_MENU) {
      currentSelection = (currentSelection + 1) % sendMenuSize;
      displaySendMenu();
    }
    delay(300);
  }
  
  // Select the current menu option
  if (btn_selc == LOW) {
    if (currentMenu == MAIN_MENU) {
      // Option 0 = Send Message, Option 1 = Received Message
      if (currentSelection == 0) {
        currentMenu = SEND_MENU;
        currentSelection = 0;
        displaySendMenu();
      } else if (currentSelection == 1) {
        currentMenu = RECEIVE_MENU;
        displayReceiveMenu();
      }
    } else if (currentMenu == SEND_MENU) {
      // Send the message corresponding to the selected option
      sendSelectedMessage();
      // Return to the main menu afterward
      currentMenu = MAIN_MENU;
      currentSelection = 0;
      displayMainMenu();
    } else if (currentMenu == RECEIVE_MENU) {
      // When in the received messages screen, a select returns to the main menu
      currentMenu = MAIN_MENU;
      currentSelection = 0;
      displayMainMenu();
    }
    delay(300);
  }
  
  // Back button: return to the main menu if not already there
  if (btn_back == LOW) {
    if (currentMenu != MAIN_MENU) {
      currentMenu = MAIN_MENU;
      currentSelection = 0;
      displayMainMenu();
      delay(300);
    }
  }
}

// ------------------------
// Setup and Loop
// ------------------------

void setup() {
  Serial.begin(115200);
  LoRa.setPins(SS, RST, DIO0);
  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PREV, INPUT_PULLUP);
  pinMode(BUTTON_SELC, INPUT_PULLUP);
  pinMode(BUTTON_BACK, INPUT_PULLUP);

  // Initialize LoRa (433MHz)
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.display();
  
  // Start with the main menu on the display
  displayMainMenu();
  Serial.println("LoRa is ready. Device ID: " + String(ID));
}

void loop() {
  // Continuously check for incoming LoRa messages
  checkForReceivedMessage();
  
  // Update menu based on button input
  updateMenu();
  
  // Optionally, allow sending custom messages via Serial
  if (Serial.available()) { 
    String message = Serial.readStringUntil('\n');
    message.trim();
    if (message.length() > 0) {
      String fullMessage = String(ID) + ": " + message;
      LoRa.beginPacket();
      LoRa.print(fullMessage);
      LoRa.endPacket();
      display.clearDisplay();
      display.setCursor(10, 20);
      display.println("Message Sent:");
      display.println(fullMessage);
      display.display();
      delay(2000);
      
      // Refresh the current menu display
      if (currentMenu == MAIN_MENU)
        displayMainMenu();
      else if (currentMenu == SEND_MENU)
        displaySendMenu();
      else if (currentMenu == RECEIVE_MENU)
        displayReceiveMenu();
    }
  }
}