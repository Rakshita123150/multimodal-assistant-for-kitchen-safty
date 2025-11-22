#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// --- I2C LCD Configuration ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Sensor Pin Configuration ---
#define DHTPIN 7
#define DHTTYPE DHT11
#define GAS_SENSOR_PIN A0
#define PIR_SENSOR_PIN 8
#define FIRE_SENSOR_PIN 9 // Active Low

// --- Alert Thresholds ---
#define GAS_ALERT_THRESHOLD 400 

// --- Timing ---
long lastUpdateTime = 0;
const long updateInterval = 2000; // Update all sensor readings every 2 seconds

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize DHT
  dht.begin();
  
  // Initialize Pins
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(FIRE_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  
  // Print static header text
  lcd.setCursor(0, 0); 
  // Print T: and H: headers (leave space for values)
  lcd.print("T:    C H:    %");
}

void loop() {
  // Read digital sensors immediately (for quick response)
  int pirState = digitalRead(PIR_SENSOR_PIN);
  int fireState = digitalRead(FIRE_SENSOR_PIN); // LOW when fire detected
  
  // --- Check for periodic sensor update ---
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    
    // 1. Read Sensors
    float h = dht.readHumidity();
    float t = dht.readTemperature(); 
    int gasValue = analogRead(GAS_SENSOR_PIN); 

    // 2. Determine Message Priority
    String line2Message = ""; 
    
    // PRIORITY 1: FIRE ALERT (The most critical)
    if (fireState == LOW) {
      line2Message = "!!! FIRE ALERT !!!";
    } 
    // PRIORITY 2: GAS Alert
    else if (gasValue > GAS_ALERT_THRESHOLD) {
      line2Message = "!!! GAS LEAK !!!";
    } 
    // PRIORITY 3: PIR Alert
    else if (pirState == HIGH) {
      line2Message = ">> MOTION ALERT!";
    }
    // PRIORITY 4: DHT Errors
    else if (isnan(h) || isnan(t)) {
      line2Message = "DHT Read Failed!";
    }
    // PRIORITY 5: Default Status
    else {
      // Default: Display Gas value on line 2
      line2Message = "Gas: " + String(gasValue) + "/1023    ";
    }
    
    // --- Display DHT Data on Line 1 (Selective Clear) ---
    if (!isnan(h) && !isnan(t)) {
      // Temperature (T: xx.xC)
      lcd.setCursor(2, 0); // Start column for Temp value
      lcd.print("      "); // Clear old Temp
      lcd.setCursor(2, 0); 
      lcd.print(t, 1);
      lcd.print((char)223); // Degree symbol
      
      // Humidity (H: xx%)
      lcd.setCursor(10, 0); // Start column for Humidity value
      lcd.print("     "); // Clear old Humidity
      lcd.setCursor(10, 0);
      lcd.print(h, 0);
    }
    
    // --- Display Status/Alert on Line 2 (Full Clear for Critical Alerts) ---
    lcd.setCursor(0, 1);
    
    // If a critical alert is active, use a full clear for maximum visibility
    if (line2Message.startsWith("!!!") || line2Message.startsWith("DHT")) {
      lcd.clear(); // Full screen clear (will cause flicker but guarantees clear alert)
      // Redraw static text
      lcd.setCursor(0, 0); 
      lcd.print("T:    C H:    %");
      lcd.setCursor(0, 1);
      lcd.print(line2Message);
      
    } else {
      // For non-critical alerts (Motion) or default status (Gas Value), only clear line 2
      lcd.setCursor(0, 1);
      lcd.print("                "); // Clear entire line 2 with spaces
      lcd.setCursor(0, 1);
      
      // If Motion is the highest priority, print it
      if (line2Message.startsWith(">> MOTION")) {
          lcd.print(line2Message);
      } else {
          // Default Gas value status (already formatted with spaces in line2Message)
          lcd.print(line2Message);
      }
    }
  } else {
    // Immediate Fire check (Highest Priority)
    if (fireState == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("!!! FIRE ALERT !!!");
        // Flash a message on line 2
        lcd.setCursor(0, 1);
        if (millis() % 500 < 250) { // Flashes every 0.5 seconds
           lcd.print("   EVACUATE NOW   ");
        } else {
           lcd.print("                "); 
        }
    }
    // Immediate Motion check (if no critical alert)
    else if (pirState == HIGH && digitalRead(FIRE_SENSOR_PIN) != LOW && analogRead(GAS_SENSOR_PIN) < GAS_ALERT_THRESHOLD) {
         lcd.setCursor(0, 1);
         lcd.print(">> MOTION ALERT! "); // Print with space to overwrite old text
    }
  }
}
