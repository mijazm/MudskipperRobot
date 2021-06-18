/*************************************************** 
This code conjures a mudskipper robot that will haunt your dreams/nightmares,
Proceed with caution because you will jump with joy when your mudskipper robot comes alive and 
you end up banging your head on a shelf right above you, consider yourself warned.

Author: Mijaz Mukundan
License: MIT (Read the one attached if you are bored)

References:
1. https://learn.adafruit.com/16-channel-pwm-servo-driver?view=all (the example from which I have remorselessly copied most of the things)

 ****************************************************/
#include<Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "us100.h"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>

#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include "ESPAsyncWebServer.h"
#include "credentials.h"

//Hardware serial pins to be used with US100 ultrasonic sensor
#define RXD2 16
#define TXD2 17

//Initialize servo driver object
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  350 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  500 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

//servo connections to the driver
int right_h = 0;//right fin horizontal movement servo
int right_v = 1;//right fin vertical movement servo
int left_h = 2; //left fin horizontal movement servo
int left_v = 4; //left fin vertical servo

//Creating US100 Ultrasonic sensor object ( the eyes of mudskipper)
US100 us100;

//Function declaration for mudskipper movement functions
boolean move_forward(uint8_t mov_speed);
boolean move_backward(uint8_t mov_speed);
boolean move_left(uint8_t mov_speed);
boolean move_right(uint8_t mov_speed);
boolean move_auto(uint8_t mov_speed);

const char *PARAM_INPUT = "speedValue";

char move_cmd = 's'; //initially mudskipper is in stop condition
String distance = "0";
int speed = 0;

unsigned long last_time;

// HTML web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <title>Mudskipper Robot Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        html {
            font-family: Helvetica;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
        }

        h1 {
            color: #0F3376;
            padding: 2vh;
        }

        p {
            font-size: 1.5rem;
        }

        .button {
            display: inline-block;
            background-color: #e7bd3b;
            border: none;
            border-radius: 50%;
            color: white;
            padding: 16px 40px;
            text-decoration: none;
            font-size: 30px;
            margin: 2px;
            cursor: pointer;
        }

        .button2 {
            background-color: #f44242;
        }
        .button3 {
            background-color: #85e967;
            border-radius: 0%;
        }
        .slidecontainer {
  width: 100%; /* Width of the outside container */
}

/* The slider itself */
.slider {
  -webkit-appearance: none;  /* Override default CSS styles */
  appearance: none;
  width: 50%; /* Full-width */
  height: 25px; /* Specified height */
  background: #d3d3d3; /* Grey background */
  outline: none; /* Remove outline */
  opacity: 0.7; /* Set transparency (for mouse-over effects on hover) */
  -webkit-transition: .2s; /* 0.2 seconds transition on hover */
  transition: opacity .2s;
}

/* Mouse-over effects */
.slider:hover {
  opacity: 1; /* Fully shown on mouse-over */
}

/* The slider handle (use -webkit- (Chrome, Opera, Safari, Edge) and -moz- (Firefox) to override default look) */
.slider::-webkit-slider-thumb {
  -webkit-appearance: none; /* Override default look */
  appearance: none;
  width: 25px; /* Set a specific slider handle width */
  height: 25px; /* Slider handle height */
  background: #04AA6D; /* Green background */
  cursor: pointer; /* Cursor on hover */
}

.slider::-moz-range-thumb {
  width: 25px; /* Set a specific slider handle width */
  height: 25px; /* Slider handle height */
  background: #04AA6D; /* Green background */
  cursor: pointer; /* Cursor on hover */
}
    </style>
</head>

<body>
    <h1>Mudskipper Controller</h1>
    </p>
    <p><button class="button" onmousedown="toggleCheckbox('forward');" ontouchstart="toggleCheckbox('forward');">&#8593;</button></a></p> 
    <p><button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');">&#8592;</button></a>
        <button class="button button2" onmousedown="toggleCheckbox('stop');" ontouchstart="toggleCheckbox('stop');">&#88;</button></a>
        <button class="button" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');">&#8594;</button></a></p>
    <p><button class="button" onmousedown="toggleCheckbox('back');" ontouchstart="toggleCheckbox('back');">&#8595;</button></a></p>
    <p><button class="button button3" onmousedown="toggleCheckbox('auto');" ontouchstart="toggleCheckbox('auto');">Zombie Mode</button></a></p>
    <p><div class="slidecontainer">
        Speed:<input type="range" onchange="updateSpeed(this)" min="0" max="255" value="0" class="slider" id="speedSlider">
      </div></p>
    <p>Distance from enemy : 
       <span id="distance"></span>
       <sup class="units">mm</sup>
    </p>
</body>
<script>
    
  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distance", true);
  xhttp.send();
  }, 2000) ;

    function updateSpeed(element){
    var speedValue = document.getElementById("speedSlider").value;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?speedValue="+speedValue, true);
    xhr.send();

    }
    function toggleCheckbox(x) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/" + x, true);
      xhr.send();
    }
   </script>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

String processor(const String& var)
{
  //Serial.println(var);
  if (var == "DISTANCE")
  {
    return distance;
  }
  else{
    return String();
  }
}



void setup() {

  
  Serial.begin(115200);

  //Setting up WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  
  //Set Connection for US100
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  us100.set_serial(&Serial2);

  Serial.println("Autonomous Mudskipper Robot has started!");
  
  //Start the servo driver
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  delay(10);
  
  // Send web page to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
    });

    server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", distance.c_str());
    });

    // Receive an HTTP GET request
    server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 'l';
      request->send(200, "text/plain", "ok");
    });

    // Receive an HTTP GET request
    server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 'r';
      request->send(200, "text/plain", "ok");
    });

    // Receive an HTTP GET request
    server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 'f';
      request->send(200, "text/plain", "ok");
    });

    // Receive an HTTP GET request
    server.on("/back", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 'b';
      request->send(200, "text/plain", "ok");
    });

    // Receive an HTTP GET request
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 's';
      request->send(200, "text/plain", "ok");
    });

    // Receive an HTTP GET request
    server.on("/auto", HTTP_GET, [](AsyncWebServerRequest *request) {
      move_cmd = 'a';
      request->send(200, "text/plain", "ok");
    });

    // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
    server.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request) {
      String inputMessage;
      // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
      if (request->hasParam(PARAM_INPUT))
      {
        inputMessage = request->getParam(PARAM_INPUT)->value();
        // sliderValue = inputMessage;
        speed = inputMessage.toInt();
      }
      else
      {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/plain", "OK");
    });

    server.onNotFound(notFound);
    server.begin();
    last_time = millis();

}


void loop() {
      speed = uint8_t(speed);
      distance = (String)us100.get_distance();
      switch (move_cmd)
      {
      case 's':
        // Serial.println("Stop");
        break;
      case 'f':
        move_forward(speed);
        // Serial.println("forward");
        break;
      case 'b':
        move_backward(speed);
        // Serial.println(", backward");
        break;
      case 'l':
        move_left(speed);
        // Serial.println(", left");
        break;
      case 'r':
        move_right(speed);
        // Serial.println(", right");
        break;
      case 'a':
        // Serial.println("Autonomy Activated");
        move_auto(speed);
      default:
        Serial.println("Stop");
      }
      distance = (String)random(6);


}

boolean move_forward(uint8_t mov_speed){
/*
This function moves the mudskipper forward by using horizontal and vertical movement 
of the two servos on each fin, think of how you would paddle with both your arms to move
forward and then get confused like I did */
uint16_t plen,plen_right,plen_left;

if(mov_speed==0){
  return false;
}

//We will use delay between the horizontal and vertical movements to control the speed
int delay_time = map(mov_speed,0,255,2000,500);

 //Move the fin from back to front  
  for ( plen_right = SERVOMAX, plen_left=SERVOMIN; plen_right > SERVOMIN && plen_left<SERVOMAX; plen_right--, plen_left++) {
    pwm.setPWM(right_h, 0, plen_right);
    pwm.setPWM(left_h, 0, plen_left);
  }
  delay(delay_time);

// Move fin from up to down
  for ( plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);
  }
  delay(delay_time);

// Move flipper from front to back
  for ( plen_right = SERVOMIN, plen_left=SERVOMAX; plen_right < SERVOMAX && plen_left > SERVOMIN; plen_right++, plen_left--) {
    pwm.setPWM(right_h, 0, plen_right);
    pwm.setPWM(left_h, 0, plen_left);
  }
  delay(delay_time);

// Move flipper from down to up
  for ( plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);

  }
  delay(delay_time);

  return true;
}


boolean move_backward(uint8_t mov_speed){
/* Function for making the mudskipper move backward*/
uint16_t plen,plen_right,plen_left;

if(mov_speed==0){
  return false;
}

//We will use delay between the horizontal and vertical movements to control the speed

int delay_time = map(mov_speed,0,255,2000,500);

// Move flipper from front to back
  for (plen_right = SERVOMIN, plen_left=SERVOMAX; plen_right < SERVOMAX && plen_left > SERVOMIN; plen_right++, plen_left--) {
    pwm.setPWM(right_h, 0, plen_right);
    pwm.setPWM(left_h, 0, plen_left);
  }
  delay(delay_time);

// Move fin from up to down
  for (plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);
  }
  delay(delay_time);

 //Move the fin from back to front  
  for (plen_right = SERVOMAX, plen_left=SERVOMIN; plen_right > SERVOMIN && plen_left<SERVOMAX; plen_right--, plen_left++) {
    pwm.setPWM(right_h, 0, plen_right);
    pwm.setPWM(left_h, 0, plen_left);
  }
  delay(delay_time);

// Move flipper from down to up
  for (plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);

  }
  delay(delay_time);

  return true;
}

boolean move_left(uint8_t mov_speed){

/* If you don't know what this function does after reading its name, I can't help you*/

if(mov_speed==0){
  return false;
}

//We will use delay between the horizontal and vertical movements to control the speed
int delay_time = map(mov_speed,0,255,2000,500);

uint16_t plen;
// Move right fin from front to back and left fin from back to front
  for (plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_h, 0, plen);
    pwm.setPWM(left_h, 0, plen);
  }
  delay(delay_time);

// Move fin from up to down
  for (plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);
  }
  delay(delay_time);

// Move left fin from front to back and right fin from back to front
  for (plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_h, 0, plen);
    pwm.setPWM(left_h, 0, plen);
  }
  delay(delay_time);

// Move flipper from down to up
  for (plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);

  }
  delay(delay_time);

  return true;
}

boolean move_right(uint8_t mov_speed){

/* I will barf if I write any more comments than this*/
uint16_t plen;

if(mov_speed==0){
  return false;
}

//We will use delay between the horizontal and vertical movements to control the speed
int delay_time = map(speed,0,255,2000,500);

// Move left fin from front to back and right fin from back to front
  for (plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_h, 0, plen);
    pwm.setPWM(left_h, 0, plen);
  }
  delay(delay_time);

// Move fin from up to down
  for ( plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);
  }
  delay(delay_time);

// Move right fin from front to back and left fin from back to front
  for ( plen = SERVOMIN; plen < SERVOMAX; plen++) {
    pwm.setPWM(right_h, 0, plen);
    pwm.setPWM(left_h, 0, plen);
  }
  delay(delay_time);

// Move flipper from down to up
  for ( plen = SERVOMAX; plen > SERVOMIN; plen--) {
    pwm.setPWM(right_v, 0, plen);
    pwm.setPWM(left_v, 0, plen);

  }
  delay(delay_time);

  return true;
}

boolean move_auto(uint8_t mov_speed){
  float dist = us100.get_distance();
  Serial.println(distance);
  if (dist >= 50){
    move_forward(mov_speed);
  }
  else
  {
    move_backward(mov_speed);
    move_left(mov_speed);
  }

 return true;
}