/*  
 *   MQTT version of 
 * ____                     _      ______ _____    _____
  / __ \                   | |    |  ____|  __ \  |  __ \               
 | |  | |_ __   ___ _ __   | |    | |__  | |  | | | |__) |__ _  ___ ___ 
 | |  | | '_ \ / _ \ '_ \  | |    |  __| | |  | | |  _  // _` |/ __/ _ \
 | |__| | |_) |  __/ | | | | |____| |____| |__| | | | \ \ (_| | (_|  __/
  \____/| .__/ \___|_| |_| |______|______|_____/  |_|  \_\__,_|\___\___|
        | |                                                             
        |_|          
 Open LED Race
 An minimalist cars race for LED strip 
 New features:
 - Red car plus fire effect
 - MQTT control Pushbutton and Speed Dasboard 

 Requeriments
    Microcontroller is ESP8266 (wifi)
 
 Download from https://github.com/cacauvicosa/openledrace

 Original OpenLedRace code 
 by gbarbarov@singulardevices.com  for Arduino day Seville 2019 

 Code made dirty and fast, next improvements in: 
 https://github.com/gbarbarov/led-race
 https://www.hackster.io/gbarbarov/open-led-race-a0331a
 https://twitter.com/openledrace
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

long randNumber;

const char* ssid = "your_wifi_net";
const char* password = "your_password";


const char* mqtt_server = "broker.mqtt-dashboard.com";  // Public Broker, pay attention in topics to avoid conflicts

#define MQTTPORT 1883
#define SPEEDUPDATE 300
// replace XX for a unique ID (one or two digits should be enough to avoid conflicts to other 
#define GREENCAR  "trackXX/greencar" // MQTT topic for Green Car Push Button
#define REDCAR  "trackXX/redcar" // MQTT topic for RED Car Push Button
#define SPEEDRED "trackXX/redspeed"
#define SPEEDGREEN "trackXX/greenspeed"

WiFiClient espClient;
PubSubClient client(espClient);
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
/*****
 * Add two pushbutton subscribe. You could add more controls....
 */
      // ... and resubscribe
      client.subscribe(GREENCAR);
      client.subscribe(REDCAR);
/*******************************************************/      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


                                                            
#include <Adafruit_NeoPixel.h>
#define MAXLED         300 // MAX LEDs actives on strip

#define PIN_LED        D3  // R 500 ohms to DI pin for WS2812 and WS2813, for WS2813 BI pin of first LED to GND  ,  CAP 1000 uF to VCC 5v/GND,power supplie 5V 2A  

int PIN_P1=0;   // Mqtt "switch" state player 1 - Set to One when Receive Mqtt package, and then reset to zero 
int PIN_P2=0;   // Mqtt "switch" state player 2  


int NPIXELS=MAXLED; // leds on track

#define COLOR1    track.Color(255,0,0)
#define COLOR2    track.Color(0,255,0)

#define LOOP_max 5

float speed1=0;
float speed2=0;
float dist1=0;
float dist2=0;

byte loop1=0;
byte loop2=0;

byte leader=0;
byte loop_max=LOOP_max; //total laps race


float ACEL=0.2;
float kf=0.015; //friction constant
float kg=0.003; //gravity constant

byte flag_sw1=0;
byte flag_sw2=0;
byte draworder=0;
 
unsigned long timestamp=0;





Adafruit_NeoPixel track = Adafruit_NeoPixel(MAXLED, PIN_LED, NEO_GRB + NEO_KHZ800);

int tdelay = 5; 

long lastMsg;


/*******
 * Fire Effect - Create a static array to store the fire effect. 
 * A random position is used to fastly generate the fire effect during the race. 
 */
uint32_t fire_color   = track.Color ( 80,  35,  00);
uint32_t off_color    = track.Color (  0,  0,  0);
uint32_t firecolor[MAXLED+LOOP_max];
void fill_fire()
{
   for(int i=0;i<MAXLED+LOOP_max;i++)
   {
      firecolor[i] = Blend(firecolor[i], fire_color);
      int r = random(80);
      uint32_t diff_color = track.Color ( r, r/2, r/2);
      firecolor[i] = Substract(firecolor[i], diff_color);
    }
  
}
uint32_t Blend(uint32_t color1, uint32_t color2)
{
uint8_t r1,g1,b1;
uint8_t r2,g2,b2;
uint8_t r3,g3,b3;
r1 = (uint8_t)(color1 >> 16),
g1 = (uint8_t)(color1 >>  8),
b1 = (uint8_t)(color1 >>  0);
r2 = (uint8_t)(color2 >> 16),
g2 = (uint8_t)(color2 >>  8),
b2 = (uint8_t)(color2 >>  0);

return track.Color(constrain(r1+r2, 0, 255), constrain(g1+g2, 0, 255), constrain(b1+b2, 0, 255));
}
uint32_t Substract(uint32_t color1, uint32_t color2)
{
uint8_t r1,g1,b1;
uint8_t r2,g2,b2;
uint8_t r3,g3,b3;
int16_t r,g,b;
r1 = (uint8_t)(color1 >> 16),
g1 = (uint8_t)(color1 >>  8),
b1 = (uint8_t)(color1 >>  0);
r2 = (uint8_t)(color2 >> 16),
g2 = (uint8_t)(color2 >>  8),
b2 = (uint8_t)(color2 >>  0);
r=(int16_t)r1-(int16_t)r2;
g=(int16_t)g1-(int16_t)g2;
b=(int16_t)b1-(int16_t)b2;
if(r<0) r=0;
if(g<0) g=0;
if(b<0) b=0;
return track.Color(r, g, b);
}




void setup() {
  track.begin(); 
  start_race();    
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, MQTTPORT);
  client.setCallback(callback);
  lastMsg = 0;
  fill_fire(); // create the static array for the fire effect
}

/**********
 *  Receive the MQTT package from red/green pushbutton . Set PIN_P0/P1
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  String Topic(topic);
  Serial.print(Topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ( Topic.equals(GREENCAR) ) {
    PIN_P1 = 1;
  }
  if ( Topic.equals(REDCAR) ) {
    PIN_P2 = 1;
  }

}

/****
 * Almost the Original Openled code from 
 */
void start_race(){for(int i=0;i<NPIXELS;i++){track.setPixelColor(i, track.Color(0,0,0));};
                  track.show();
                  delay(2000);
                  track.setPixelColor(12, track.Color(0,255,0));
                  track.setPixelColor(11, track.Color(0,255,0));
                  track.show();
                  track.setPixelColor(12, track.Color(0,0,0));
                  track.setPixelColor(11, track.Color(0,0,0));
                  track.setPixelColor(10, track.Color(255,255,0));
                  track.setPixelColor(9, track.Color(255,255,0));
                  track.show();
                  track.setPixelColor(9, track.Color(0,0,0));
                  track.setPixelColor(10, track.Color(0,0,0));
                  track.setPixelColor(8, track.Color(255,0,0));
                  track.setPixelColor(7, track.Color(255,0,0));
                  track.show();
                  timestamp=0;              
                 };

void draw_car1(void){int j=random(300); int k = random(5); 
      for(int i=1+k;i<=loop1+1+k;i++){
              track.setPixelColor(((word)dist1 % NPIXELS)+i, track.Color(255-i*20,0,0));
         }                   
      for(int i=0;i<1+k;i++){
              track.setPixelColor(((word)dist1 % NPIXELS)+i, firecolor[(((word)dist1+j) % NPIXELS)+i]);
         }                   
  }

void draw_car2(void){for(int i=0;i<=loop2;i++){track.setPixelColor(((word)dist2 % NPIXELS)+i, track.Color(0,255-i*20,0));};            
 }

char msg[20];
   
void loop() {
    for(int i=0;i<NPIXELS;i++){track.setPixelColor(i, track.Color(0,0,0));};
    if (!client.connected()) {
    reconnect();
    }
    client.loop();

    if ( PIN_P1==1 ) {PIN_P1=0;speed1+=ACEL;};
    
    speed1-=speed1*kf; 
    
    if ( PIN_P2==1 ) {PIN_P2=0;speed2+=ACEL;};
       
    speed2-=speed2*kf; 

    long now = millis();
    if (now - lastMsg > SPEEDUPDATE) {
       lastMsg = now;
       snprintf (msg, 20, "%f", speed1*100);
       client.publish(SPEEDGREEN, msg);
       Serial.println(msg);
       snprintf (msg, 20, "%f", speed2*100);
       client.publish(SPEEDRED, msg);
       Serial.println(msg);       
   }

        
    dist1+=speed1;
    dist2+=speed2;

    if (dist1>dist2) {leader=1;} 
    if (dist2>dist1) {leader=2;};
      
    if (dist1>NPIXELS*loop1) {loop1++;};
    if (dist2>NPIXELS*loop2) {loop2++;};

    if (loop1>loop_max) {for(int i=0;i<NPIXELS;i++){track.setPixelColor(i, track.Color(0,255,0));}; track.show();
                                                    loop1=0;loop2=0;dist1=0;dist2=0;speed1=0;speed2=0;timestamp=0;
                                                    start_race();
                                                   }
    if (loop2>loop_max) {for(int i=0;i<NPIXELS;i++){track.setPixelColor(i, track.Color(255,0,0));}; track.show();
                                                    loop1=0;loop2=0;dist1=0;dist2=0;speed1=0;speed2=0;timestamp=0;
                                                    start_race();
                                                   }
    if ((millis() & 512)==(512*draworder)) {if (draworder==0) {draworder=1;}
                          else {draworder=0;}   
                         }; 

    if (draworder==0) {draw_car1();draw_car2();}
        else {draw_car2();draw_car1();}   
                 
    track.show(); 
    delay(tdelay);
    
    
}
