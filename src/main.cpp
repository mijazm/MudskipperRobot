/*************************************************** 
This code conjures a mudskipper robot that will haunt your dreams/nightmares,
Proceed with caution because you will jump with joy when your mudskipper robot comes alive and 
you end up banging your head on a shelf right above you, consider yourself warned.

Author: Mijaz Mukundan
License: MIT (Read the one attached if you are bored and have nothing else to do)

References:
1. https://randomnerdtutorials.com/esp32-web-server-arduino-ide/ (This tutorial is awesome if you haven't
// implemented a webserver before, I have shamelessly copied most of the stuff from there)

2. https://dronebotworkshop.com/servo-motors-with-arduino/ (If you want to learn how to make
servo motors work from a guy who knows what he is doing)
 ****************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESP32Servo.h>

#include "ESPAsyncWebServer.h"

//Do me a favour and edit this file with your WiFi credentials
#include "credentials.h"

// Check out this webpage and edit it as per your needs, or simple use it and marvel at its
// obnoxious elegance
#include "webpage.h"

// Calling the library for US100 ultrasonic sensor, which I wrote and probably could use some work,
// which I am not going to do.
#include "us100.h"

//Hardware serial pins to be used with US100 ultrasonic sensor
#define RXD2 16
#define TXD2 17

//Initialize servo driver object
// Change the following definitions as per your design and arm poisitions, its a bit of a
// trial and error, I recommend you control the servos one by one after you have done
// assembling the fins to get the right values.

#define SERVOMIN_V 20 // This is the angle for down position of \
                      // vertical movement servos
#define SERVOMAX_V 90 // This is the angle for up position of
                      // vertical movement servos

#define SERVOMAX_H_R 90 // This is the angle for front position of
                        // right horizrontal movement servos
#define SERVOMIN_H_R 0  // This is the angle for back position of
                        // right horizrontal movement servos

#define SERVOMAX_H_L 180 // This is the angle for front position of
                         // left horizrontal movement servos
#define SERVOMIN_H_L 90  // This is the angle for back position of
                         // left horizrontal movement servos

//servo pin connections
uint8_t left_h_pin = 25;  //right fin horizontal movement servo
uint8_t left_v_pin = 26;  //right fin vertical movement servo
uint8_t right_h_pin = 32; //left fin horizontal movement servo
uint8_t right_v_pin = 33; //left fin vertical servo

//Define the servo object, we need four servos so we will define 4 objects duh
Servo right_h, right_v, left_h, left_v;

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

// HTML web page from the very creatively named "webpage.h"
const char *index_html = webpage_html;

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

String processor(const String &var)
{
  //Serial.println(var);
  if (var == "DISTANCE")
  {
    return distance;
  }
  else
  {
    return String();
  }
}

void setup()
{

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

  //Initialize the servos

  // Allow allocation of all timers, don't ask me why the library recommends it
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  right_h.attach(right_h_pin);
  right_h.setPeriodHertz(50); // standard 50 hz servo

  right_v.attach(right_v_pin);
  right_v.setPeriodHertz(50); // standard 50 hz servo

  left_h.attach(left_h_pin);
  left_h.setPeriodHertz(50); // standard 50 hz servo

  left_v.attach(left_v_pin);
  left_v.setPeriodHertz(50); // standard 50 hz servo

  delay(10);

  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", distance.c_str()); });

  // Receive an HTTP GET request
  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 'l';
              request->send(200, "text/plain", "ok");
            });

  // Receive an HTTP GET request
  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 'r';
              request->send(200, "text/plain", "ok");
            });

  // Receive an HTTP GET request
  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 'f';
              request->send(200, "text/plain", "ok");
            });

  // Receive an HTTP GET request
  server.on("/back", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 'b';
              request->send(200, "text/plain", "ok");
            });

  // Receive an HTTP GET request
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 's';
              request->send(200, "text/plain", "ok");
            });

  // Receive an HTTP GET request
  server.on("/auto", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              move_cmd = 'a';
              request->send(200, "text/plain", "ok");
            });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String inputMessage;
              // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
              if (request->hasParam(PARAM_INPUT))
              {
                inputMessage = request->getParam(PARAM_INPUT)->value();
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

void loop()
{
  speed = uint8_t(speed);
  distance = (String)us100.get_distance();
  switch (move_cmd)
  {
  case 's':
    // Serial.println("Stop");
    delay(100);
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

boolean move_forward(uint8_t mov_speed)
{
  /*
This function moves the mudskipper forward by using horizontal and vertical movement 
of the two servos on each fin, think of how you would paddle with both your arms to move
forward and then get confused like I did */
  uint16_t pos, pos_right, pos_left;

  if (mov_speed == 0)
  {
    return false;
  }

  //We will use delay between the horizontal and vertical movements to control the speed
  int delay_time = map(mov_speed, 0, 255, 2000, 500);

  //Move the fin from back to front
  for (pos_right = SERVOMIN_H_R, pos_left = SERVOMIN_H_L; pos_right < SERVOMAX_H_R && pos_left < SERVOMAX_H_L; pos_right++, pos_left++)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from up to down
  for (pos = SERVOMAX_V; pos > SERVOMIN_V; pos--)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  // Move fin from front to back
  for (pos_right = SERVOMAX_H_R, pos_left = SERVOMAX_H_L; pos_right > SERVOMIN_H_R && pos_left > SERVOMIN_H_L; pos_right--, pos_left--)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from down to up
  for (pos = SERVOMIN_V; pos < SERVOMAX_V; pos++)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  return true;
}

boolean move_backward(uint8_t mov_speed)
{
  /* Function for making the mudskipper move backward like a wuss*/
  uint16_t pos, pos_right, pos_left;

  if (mov_speed == 0)
  {
    return false;
  }

  //We will use delay between the horizontal and vertical movements to control the speed
  int delay_time = map(mov_speed, 0, 255, 2000, 500);

  // Move fin from front to back
  for (pos_right = SERVOMAX_H_R, pos_left = SERVOMAX_H_L; pos_right > SERVOMIN_H_R && pos_left > SERVOMIN_H_L; pos_right--, pos_left--)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from up to down
  for (pos = SERVOMAX_V; pos > SERVOMIN_V; pos--)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  //Move the fin from back to front
  for (pos_right = SERVOMIN_H_R, pos_left = SERVOMIN_H_L; pos_right < SERVOMAX_H_R && pos_left < SERVOMAX_H_L; pos_right++, pos_left++)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from down to up
  for (pos = SERVOMIN_V; pos < SERVOMAX_V; pos++)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  return true;
}

boolean move_left(uint8_t mov_speed)
{

  /* If you don't know what this function does after reading its name, I can't help you*/

  if (mov_speed == 0)
  {
    return false;
  }

  //We will use delay between the horizontal and vertical movements to control the speed
  int delay_time = map(mov_speed, 0, 255, 2000, 500);

  uint16_t pos, pos_left, pos_right;
  // Move right fin from front to back and left fin from back to front
  for (pos_right = SERVOMAX_H_R, pos_left = SERVOMIN_H_L; pos_right > SERVOMIN_H_R && pos_left < SERVOMAX_H_L; pos_right--, pos_left++)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from up to down
  for (pos = SERVOMAX_V; pos > SERVOMIN_V; pos--)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  // Move left fin from front to back and right fin from back to front
  for (pos_right = SERVOMIN_H_R, pos_left = SERVOMAX_H_L; pos_right < SERVOMAX_H_R && pos_left > SERVOMIN_H_L; pos_right++, pos_left--)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from down to up
  for (pos = SERVOMIN_V; pos < SERVOMAX_V; pos++)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  return true;
}

boolean move_right(uint8_t mov_speed)
{
  /* I will barf if I write any more comments than this*/

  uint16_t pos, pos_left, pos_right;

  if (mov_speed == 0)
  {
    return false;
  }

  //We will use delay between the horizontal and vertical movements to control the speed
  int delay_time = map(speed, 0, 255, 2000, 500);

  // Move left fin from front to back and right fin from back to front
  for (pos_right = SERVOMIN_H_R, pos_left = SERVOMAX_H_L; pos_right < SERVOMAX_H_R && pos_left > SERVOMIN_H_L; pos_right++, pos_left--)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from up to down
  for (pos = SERVOMAX_V; pos > SERVOMIN_V; pos--)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);

  // Move right fin from front to back and left fin from back to front
  for (pos_right = SERVOMAX_H_R, pos_left = SERVOMIN_H_L; pos_right > SERVOMIN_H_R && pos_left < SERVOMAX_H_L; pos_right--, pos_left++)
  {
    right_h.write(pos_right);
    left_h.write(pos_left);
  }
  delay(delay_time);

  // Move fin from down to up
  for (pos = SERVOMIN_V; pos < SERVOMAX_V; pos++)
  {
    right_v.write(pos);
    left_v.write(pos);
  }
  delay(delay_time);
  return true;
}

boolean move_auto(uint8_t mov_speed)
{
  // This will make the mudskipper wander around like a zombie without purpose
  // which is basically what zombies do
  // with a proclivity to take a left turn on nearing an obstacle, why not right? because
  // I wrote the code.
  float dist = us100.get_distance();
  Serial.println(distance);
  if (dist >= 50)
  {
    move_forward(mov_speed);
  }
  else
  {
    move_backward(mov_speed);
    move_left(mov_speed);
  }

  return true;
}

// The end