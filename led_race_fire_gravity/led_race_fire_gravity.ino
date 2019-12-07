/*  
 *   Fire version of 
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

 Requeriments
    Microcontroller is ESP8266
 
 Download from https://github.com/cacauvicosa/openledrace

 Original OpenLedRace code 
 by gbarbarov@singulardevices.com  for Arduino day Seville 2019 

 Code made dirty and fast, next improvements in: 
 https://github.com/gbarbarov/led-race
 https://www.hackster.io/gbarbarov/open-led-race-a0331a
 https://twitter.com/openledrace
*/


long randNumber;



                                                            
#include <Adafruit_NeoPixel.h>
#define MAXLED         300 // MAX LEDs actives on strip

byte  gravity_map[MAXLED];     
void set_ramp(byte H,byte a,byte b,byte c)
{for(int i=0;i<(b-a);i++){gravity_map[a+i]=127-i*((float)H/(b-a));};
 gravity_map[b]=127; 
 for(int i=0;i<(c-b);i++){gravity_map[b+i+1]=127+H-i*((float)H/(c-b));};
}

#define PIN_LED        D3  // R 500 ohms to DI pin for WS2812 and WS2813, for WS2813 BI pin of first LED to GND  ,  CAP 1000 uF to VCC 5v/GND,power supplie 5V 2A  
#define PIN_P1         D2   // switch player 1 to PIN and GND
#define PIN_P2         D1   // switch player 2 to PIN and GND 


int NPIXELS=MAXLED; // leds on track

#define COLOR1    track.Color(0,255,0)
#define COLOR2    track.Color(255,0,0)

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
    for(int i=0;i<NPIXELS;i++){gravity_map[i]=127;};
   set_ramp(10,190,200,210);    // ramp centred in LED 100 with 10 led fordward and 10 backguard 
   set_ramp(10,40,50,60);     
 
  track.begin();
   pinMode(PIN_P1,INPUT_PULLUP); 
  pinMode(PIN_P2,INPUT_PULLUP);  
 
  start_race();    
  lastMsg = 0;
  fill_fire(); // create the static array for the fire effect
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

     if ( (flag_sw1==1) && (digitalRead(PIN_P1)==0) ) {flag_sw1=0;speed1+=ACEL;};
    if ( (flag_sw1==0) && (digitalRead(PIN_P1)==1) ) {flag_sw1=1;};

     if ((gravity_map[(word)dist1 % NPIXELS])<127) speed1-=kg*(127-(gravity_map[(word)dist1 % NPIXELS]));
    if ((gravity_map[(word)dist1 % NPIXELS])>127) speed1+=kg*((gravity_map[(word)dist1 % NPIXELS])-127);
    

 
    speed1-=speed1*kf; 
    
    if ( (flag_sw2==1) && (digitalRead(PIN_P2)==0) ) {flag_sw2=0;speed2+=ACEL;};
    if ( (flag_sw2==0) && (digitalRead(PIN_P2)==1) ) {flag_sw2=1;};

   
    if ((gravity_map[(word)dist2 % NPIXELS])<127) speed2-=kg*(127-(gravity_map[(word)dist2 % NPIXELS]));
    if ((gravity_map[(word)dist2 % NPIXELS])>127) speed2+=kg*((gravity_map[(word)dist2 % NPIXELS])-127);

        
    speed2-=speed2*kf; 
        
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
