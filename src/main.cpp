#include <spi.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>    // Include the DHT library
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <Arduino.h>
#include <Wire.h>
#include <SdFat.h>

// #include "index.h" // Our HTML webpage contents with javascripts

#define LED 2        // On board LED
#define DHTpin 0     // D3 of NodeMCU is GPIO0


IPAddress local_IP(192, 168, 1, 1);  // Define the desired static IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

DHT dht(DHTpin, DHT11); // Initialize DHT object for DHT11 sensor
Adafruit_BMP085 bmp;
Adafruit_MPU6050 mpu;

/* Create new sensor events */
sensors_event_t a, g, temp;
SdFat sd;
SdFile root;
SdFile tempfile;

const int chipSelectPin = 24;

const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>

<title>E-cube DashBoard/</title>
  <!--For offline ESP graphs see this tutorial https://circuits4you.com/2018/03/10/esp8266-jquery-and-ajax-web-server/ -->
 
  <style>
  canvas{
    -moz-user-select: none;
    -webkit-user-select: none;
    -ms-user-select: none;
  }

  /* Data Table Styling */
  #dataTable {
    font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
    border-collapse: collapse;
    width: 100%;
  }

  #dataTable td, #dataTable th {
    border: 1px solid #ddd;
    padding: 8px;
  }

  #dataTable tr:nth-child(even){background-color: #f2f2f2;}

  #dataTable tr:hover {background-color: #ddd;}

  #dataTable th {
    padding-top: 12px;
    padding-bottom: 12px;
    text-align: left;
    background-color: #ed1c24;
    color: white;
  }
  </style>
</head>

<body>
    
  <table id="dataTable">
    <tr><th>Time</th><th>Altitude (m)</th><th>Pressure (pa)</th><th>Temperaure (&deg;C)</th><th>Humidity (%)</th><th>Accel.X</th><th>Accel.Y</th><th>Accel.Z</th></tr>
  </table>
</div>
<br>
<br>  

<script>
//Graphs visit: https://www.chartjs.org
var ADCvalues = [];
var Tvalues = [];
var Hvalues = [];
var timeStamp = [];
var pressure = [];


//On Page load show graphs
window.onload = function() {
  console.log(new Date().toLocaleTimeString());
};

//Ajax script to get ADC voltage at every 5 Seconds 
//Read This tutorial https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/

setInterval(function() {
  // Call a function repetatively with 5 Second interval
  getData();
}, 200); //5000mSeconds update rate
 
function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     //Push the data in array
  var time = new Date().toLocaleTimeString();
  var txt = this.responseText;
  var obj = JSON.parse(txt); //Ref: https://www.w3schools.com/js/js_json_parse.asp
      ADCvalues.push(obj.Altitude);
      Tvalues.push(obj.Temperature);
      Hvalues.push(obj.Humidity);
      timeStamp.push(time);
  //Update Data Table
    var table = document.getElementById("dataTable");
    var row = table.insertRow(1); //Add after headings
    var cell1 = row.insertCell(0);
    var cell2 = row.insertCell(1);
    var cell3 = row.insertCell(2);
    var cell4 = row.insertCell(3);
    var cell5 = row.insertCell(4);
    var cell6 = row.insertCell(5);
    var cell7 = row.insertCell(6);
    var cell8 = row.insertCell(7);
    cell1.innerHTML = time;
    cell2.innerHTML = obj.Altitude;
    cell3.innerHTML = obj.Pressure;
    cell4.innerHTML = obj.Temperature;
    cell5.innerHTML = obj.Humidity;
    cell6.innerHTML = obj.AccelX;
    cell7.innerHTML = obj.AccelY;
    cell8.innerHTML = obj.AccelZ;
    
    }
  };
  xhttp.open("GET", "readADC", true); //Handle readADC server on ESP8266
  xhttp.send();
}
    
</script>

</html>

)=====";


// SSID and Password of your WiFi router
const char* ssid = "satellite";
const char* password = "satellite";

ESP8266WebServer server(80); // Server on port 80
void printAvailableData(void);
void printheader(void);
void printdata(void);


void handleRoot() {
  String s = MAIN_page;
  server.send(200, "text/html", s);
}

float humidity, temperature;

void handleADC() {
  // int a = analogRead(A0);
  int altitude = bmp.readAltitude();
  int pressure = bmp.readPressure();

  humidity = dht.readHumidity();      // Read humidity from DHT sensor
  temperature = dht.readTemperature(); // Read temperature from DHT sensor

  String data = "{\"Pressure\":\"" + String(pressure) + "\", \"Altitude\":\"" + String(altitude) + "\", \"Temperature\":\"" + String(temperature) + "\", \"Humidity\":\"" + String(humidity) + "\",\"AccelX\":\"" + String(float(a.acceleration.x),6) + "\", \"AccelY\":\"" + String(float(a.acceleration.y),6) + "\", \"AccelZ\":\"" + String(float(a.acceleration.z),6) + "\"}";

  digitalWrite(LED, !digitalRead(LED));
  server.send(200, "text/plain", data);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  dht.begin(); // Initialize the DHT sensor

  if (!sd.begin(chipSelectPin)) {
    Serial.println("SD card initialization failed!");
  }
  delay(100);

  if (bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    // while (1) {}
  }

  // WiFi.begin(ssid, password); (if want to use it to connect to an established network)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  delay(10);
  Serial.println(WiFi.status());
 
  pinMode(LED, OUTPUT);
  delay(10);

  printheader();
  delay(10);
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  server.on("/", handleRoot);
  server.on("/readADC", handleADC);
 
  server.begin();
  Serial.println("HTTP server started");

  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Adafruit MPU6050 test!");
  
  // initialize
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    
  }
  mpu.setCycleRate(MPU6050_CYCLE_20_HZ);
  mpu.enableSleep(false);
  mpu.enableCycle(true);

  Serial.println("MPU6050 Found!");
}

void loop() {
  server.handleClient();
  delay(10);
  printAvailableData();
  delay(10);
  printdata();
}

void printAvailableData(void) {

  /* Populate the sensor events with the readings*/
  mpu.getEvent(&a, &g, &temp);
  /* Print out acceleration data in a plotter-friendly format */
  Serial.print(a.acceleration.x);
  Serial.print(",");
  Serial.print(a.acceleration.y);
  Serial.print(",");
  Serial.print(a.acceleration.z);
  Serial.println("");
}

void printdata(void) {
  mpu.getEvent(&a, &g, &temp);
  tempfile.open("tmp_log.txt",O_WRITE | O_AT_END);  
  tempfile.print("    ");
  tempfile.print(a.acceleration.x);
  tempfile.print("    ");
  tempfile.print(a.acceleration.y);
  tempfile.print("    ");
  tempfile.print(a.acceleration.z);
  tempfile.print("    ");
  tempfile.close();
}

void printheader() {
  if (tempfile.open("tmp_log.txt",O_WRITE | O_CREAT)){
    tempfile.print("   X-axis      Y-axis      Z-axis   ");
    tempfile.close(); 
  }
}