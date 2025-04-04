#define BATT1_PIN 34
#define BATT2_PIN 35

#define CURRENT_PIN 32

#define SWITCH_CHARGE_PIN  14
#define SWITCH_OUTPUT_PIN  13
#define SWITCH_BALANCE_PIN 12

#define BALANCE_FREQ 5000
#define BALANCE_RES  8

#include <esp_now.h>
#include <WiFi.h>

typedef enum bms_state {
    IDLE,         // 0 
    DISCONNECTED, // 1
    CHARGING,     // 2
    DISCHARGING,  // 3
    BALANCING     // 4
};

bms_state current_state, prev_state;

typedef struct sender_message {
  unsigned int state;
  char err[20];
  float batt1_voltage;
  float batt2_voltage;
  float total_voltage;
  float load_current;
  bool isBalancing;
  bool isCharging;
  bool isOutputLoad;
  bool hasLoad;
} sender_message;

sender_message bms_data;

uint8_t broadcastAddress[] = {0x14, 0x2b, 0x2f, 0xeb, 0xa7, 0x24};
esp_now_peer_info_t peerInfo;

float load_current;

float batt1_voltage , batt2_voltage, total_series_voltage;
float first_volt_factor = 0.3125;
float second_volt_factor = 0.3666;

bool switch_output = 0;
bool switch_charge = 0;
bool switch_balance = 0;

unsigned int serial_command;

static int esp_now_timer;
static int balance_timer;

void read_battery_voltages();
void read_load_curent();
void update_system_state();
void update_system();
void get_serial_command();

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(112500);

  pinMode(BATT1_PIN, INPUT);
  pinMode(BATT2_PIN, INPUT);

  pinMode(CURRENT_PIN, INPUT);

  pinMode(SWITCH_CHARGE_PIN, OUTPUT);
  pinMode(SWITCH_BALANCE_PIN, OUTPUT);
  pinMode(SWITCH_OUTPUT_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_timer = millis();
  balance_timer = millis();

  read_battery_voltages();
  read_load_current();
  update_system_state();
  update_system();

  Serial.print("System Status has been set to -> ");
  Serial.println(current_state);
}

void loop() {
  read_battery_voltages();
  read_load_current();
  //get_serial_command();
  update_system_state();
  update_system();

  digitalWrite(SWITCH_CHARGE_PIN, switch_charge);
  digitalWrite(SWITCH_OUTPUT_PIN, switch_output); 
  digitalWrite(SWITCH_BALANCE_PIN, switch_balance);

  if(esp_now_timer - millis() >= 1000){
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bms_data, sizeof(bms_data));
    
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    esp_now_timer = millis();
  }
  delay(1000);
}

//Only For Demonstration Purpose
void get_serial_command(){
  if (Serial.available() > 0) {
    serial_command = Serial.read();

    Serial.print("Received: ");
    Serial.println(serial_command);

    switch (serial_command){
      case 48:
        current_state = IDLE;
      break;
      case 49:
        current_state = DISCONNECTED;
      break;
      case 50:
        current_state = CHARGING;
      break;
      case 51:
        current_state = DISCHARGING;
      break;
      case 52:
        current_state = BALANCING;
      break;
      default:
        current_state = IDLE;
      break;
    }
    while (Serial.available()) {
      Serial.read(); // Discard without processing
    }
  }
}

void read_battery_voltages(){
  float adc_voltage = 0.0;

  adc_voltage = analogRead(BATT1_PIN);
  batt1_voltage = ((adc_voltage * 3.3 ) / 4095) / first_volt_factor;
  batt1_voltage = batt1_voltage + 0.1 * batt1_voltage;

  adc_voltage = analogRead(BATT2_PIN);
  total_series_voltage = ((adc_voltage * 3.3) / 4095) / second_volt_factor;
  batt2_voltage = total_series_voltage - batt1_voltage;

  bms_data.batt1_voltage = batt1_voltage;
  bms_data.batt2_voltage = batt2_voltage;
  bms_data.total_voltage = total_series_voltage;

  // Serial.print("Battery 1 Voltage -> ");
  // Serial.println(batt1_voltage);
  // Serial.print("Battery 2 Voltage -> ");
  // Serial.println(batt2_voltage);
  // Serial.print("Series Voltage -> ");
  // Serial.println(total_series_voltage);

}

void read_load_current(){
  load_current = 50;
  bms_data.load_current = 50;

  load_current = ((analogRead(CURRENT_PIN) * 3.3) / 4095) / 0.6666;
  //load_current = ((load_current - 1.2) / 0.185) * 10 ;

  Serial.println(load_current);

}

void update_system_state(){
  if(total_series_voltage <= 0.0 ){
    prev_state = current_state;
    current_state = DISCONNECTED;
  }else if(current_state == CHARGING){
    if(batt1_voltage >= 2.8 || batt2_voltage >= 2.8){
      current_state = prev_state;
    }else{
      current_state = CHARGING;
    }
  }else{
    if(total_series_voltage > 4.5 ){
      if(load_current > 10){
        prev_state = current_state;
        current_state = DISCHARGING;
      }else if(batt1_voltage < (batt2_voltage - (0.1 * batt2_voltage)) || batt1_voltage - (0.1 * batt1_voltage) > batt2_voltage){
        prev_state = current_state;
        current_state = BALANCING;
      }else{
        prev_state = current_state;
        current_state = IDLE;
      }
    }else if(total_series_voltage <= 4.5){
      prev_state = current_state;
      current_state = CHARGING;
    }else{
      prev_state = current_state;
      current_state = IDLE;
    }
  }
}

void update_system(){
  switch (current_state) {
    case IDLE:
      Serial.println("SYSTEM STATUS -> IDLE");
      switch_charge = 0;
      switch_output = 0;
      switch_balance = 0;
      
      bms_data.state = 0;
      bms_data.isCharging = 0;
      bms_data.isBalancing = 0;
      bms_data.isOutputLoad = 1;
      bms_data.hasLoad = 0;

    break;
    case DISCONNECTED:
      Serial.println("SYSTEM STATUS -> DISCONNECTED");
      switch_charge = 0;
      switch_output = 1;
      switch_balance = 0;

      bms_data.state = 1;
      bms_data.isCharging = 0;
      bms_data.isBalancing = 0;
      bms_data.isOutputLoad = 0;
      bms_data.hasLoad = 0;

    break;
    case CHARGING:
      Serial.println("SYSTEM STATUS -> CHARGING");
      switch_charge = 1;
      switch_output = 1;
      switch_balance = 0;

      bms_data.state = 2;
      bms_data.isCharging = 1;
      bms_data.isBalancing = 0;
      bms_data.isOutputLoad = 0;
      bms_data.hasLoad = 0;

    break;
    case DISCHARGING:
      Serial.println("SYSTEM STATUS -> DISCHARGING");
      switch_charge = 0;
      switch_output = 0;
      switch_balance = 0;
      
      bms_data.state = 3;
      bms_data.isCharging = 0;
      bms_data.isBalancing = 0;
      bms_data.isOutputLoad = 1;
      bms_data.hasLoad = 1;

    break;
    case BALANCING:
      Serial.println("SYSTEM STATUS -> BALANCING");
      switch_charge = 0;
      switch_output = 1;

      if(balance_timer - millis() > 500){
        switch_balance = !switch_balance;
        balance_timer = millis();
      }

      //switch_balance = 1;

      bms_data.state = 4;
      bms_data.isCharging = 0;
      bms_data.isBalancing = 1;
      bms_data.isOutputLoad = 0;
      bms_data.hasLoad = 0;

    break;
    default:
      Serial.println("SYSTEM is DEFAULTING");
      switch_charge = 0;
      switch_output = 0;
      switch_balance = 0;

      bms_data.state = 0;
      bms_data.isCharging = 0;
      bms_data.isBalancing = 0;
      bms_data.isOutputLoad = 0;
      bms_data.hasLoad = 0;

    break;
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
