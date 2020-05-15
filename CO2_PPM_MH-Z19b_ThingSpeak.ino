#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>


SoftwareSerial co2Serial(D3, D4); // define MH-Z19 RX TX D3 (GPIO0) and D4 (GPIO2)
unsigned long startTime = millis();

const char* ssid     = "Your WiFi SSID";
const char* password = "Your WiFi Password";
const char* host = "api.thingspeak.com";
String apiKey = "Your Thingspeak API write key of your Thingspeak channel"; // thingspeak.com api key goes here

WiFiClient client;

void setup() {
  Serial.begin(9600);
  co2Serial.begin(9600);
  // pinMode(10, INPUT); //using GPIO10 for pulse read with pulseIn()
  connectToWiFi();
}

void loop() {
  Serial.println("------------------------------");
  Serial.print("Time from start: ");
  Serial.print((millis() - startTime) / 1000);
  Serial.println(" s");
  int ppm_uart = readCO2UART();
  // int ppm_pwm = readCO2PWM();  
  delay(15000);
}

int readCO2UART() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  char response[9];

  Serial.println("Sending CO2 request...");
  co2Serial.write(cmd, 9); //request PPM CO2
  

  // clear the buffer
  memset(response, 0, 9);
  int i = 0;
  while (co2Serial.available() == 0) {
  
       Serial.print("Waiting for response ");
       Serial.print(i);
        Serial.println(" s");
    delay(1000);
    i++;
  }
  if (co2Serial.available() > 0) {
    co2Serial.readBytes(response, 9);
  
  }
  // print out the response in hexa
  for (int i = 0; i < 9; i++) {
    Serial.print(String(response[i], HEX));
    Serial.print("   ");
  }
  Serial.println("");

  // checksum
  byte check = getCheckSum(response);
  if (response[8] != check) {
    Serial.println("Checksum not OK!");
    Serial.print("Received: ");
    Serial.println(response[8]);
    Serial.print("Should be: ");
    Serial.println(check);
  }

  // ppm
  int ppm_uart = 256 * (int)response[2] + response[3];
  Serial.print("UART CO2 PPM: ");
  Serial.println(ppm_uart);

  // temp
  byte temp = response[4] - 40;
  Serial.print("Sensor Temperature: ");
  Serial.println(temp);

  // status
  byte status = response[5];
  Serial.print("Status: ");
  Serial.println(status);
  if (status == 0x40) {
    Serial.println("Status OK");
  }

if (client.connect(host,80)) {
    String postStr = apiKey;
    postStr +="&field1=";
    postStr += String(ppm_uart);
    postStr +="&field2=";
    postStr += String(temp);
    postStr += "\r\n\r\n";
  
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
  Serial.print("Uploaded to Thingspeak ");

  return ppm_uart;


}

byte getCheckSum(char *packet) {
  byte i;
  unsigned char checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}

// int readCO2PWM() {
//  unsigned long th, tl, ppm_pwm = 0;
//  do {
//    th = pulseIn(10, HIGH, 1004000) / 1000;
//    tl = 1004 - th;
//    ppm_pwm = 5000 * (th - 2) / (th + tl - 4);
//  } while (th == 0);
//  Serial.print("PWM CO2 PWM: ");
//  Serial.println(ppm_pwm);
//  return ppm_pwm;
// }

void connectToWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  Serial.print("Connecting to ");
  Serial.println(ssid); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
