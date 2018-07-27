#include <Wire.h>
#include <SPI.h>
#include <HP203B.h>
#include <SSD1306Spi.h>

//#define USE_SOFTWAREI2C
#ifdef USE_SOFTWAREI2C 
#include <SoftwareI2C.h>
SoftwareI2C sWire(14, 16);
HP203B<SoftwareI2C> hp203b(sWire);
#else
HP203B<> hp203b(Wire);
//HP203B<TwoWire> hp203b(Wire); // TwoWire is the default class, so this is the same as above
#endif

#define OLED_WIDTH 64
#define OLED_HEIGHT 32
SSD1306Spi oled(OLED_RST, OLED_DC, OLED_CS); // (pin_rst, pin_dc, pin_cs)

float tpa[3];
uint32_t tPage;
bool B1_isPressed = false;
uint8_t page_num = 0;
const uint8_t page_count = 2;

void draw_page(uint8_t idx = 0);

void setup()
{
  Wire.begin();
  
  Serial.begin(9600);
  Serial.println(__FILE__);

  pinMode(PIN_BUTTON1, INPUT_PULLUP);

  hp203b.init();

  oled.setScreenSize(OLED_WIDTH, OLED_HEIGHT);
  oled.init();
  oled.flipScreenVertically();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  draw_page(page_num++);
  delay(3000); // show splash for 3s
  tPage = millis();
}


void loop()
{
  if (!B1_isPressed & !digitalRead(PIN_BUTTON1)) // timer used for button debounce
  {
    page_num = (page_num + 1 < page_count)?page_num+1:0;
  }
  B1_isPressed = !digitalRead(PIN_BUTTON1);
  
  if (millis() - tPage > 20) // 20ms = 50Hz
  {
    tPage = millis();
    draw_page(page_num);
  }
  yield();
}

void float2chars(float &in, char (&out)[5])
{
  bool sign_bit = (in < 0);
  uint16_t tmp = sign_bit?(-in * 10):(in * 10);
  out[0] = (sign_bit)?'-':' ';
  out[1] = char('0' + (tmp / 10));
  out[2] = '.';
  out[3] = char('0' + (tmp % 10));
  out[4] = '\0';
}

void draw_page(uint8_t idx)
{
  switch(idx)
  {
    case 1:
      page_tpa(); break;
    default:
      page_startup();
    break;
  }
}

void page_startup()
{
  oled.clear();
  oled.drawString(0,0,"github.com/");
  oled.drawString(0,10,"micooke");
  oled.drawString(0,20,__TIME__);
  oled.display();
}

void page_tpa()
{
    char fltBuf[5];
    
    hp203b.getTempPresAlt(tpa);
    
    oled.clear();
    float2chars(tpa[0],fltBuf);
    oled.drawString(0,0,"T:"); oled.drawString(10,0,fltBuf);
    float2chars(tpa[1],fltBuf);
    oled.drawString(0,10,"P:"); oled.drawString(10,10,fltBuf);
    float2chars(tpa[2],fltBuf);
    oled.drawString(0,20,"A:"); oled.drawString(10,20,fltBuf);
    oled.display();  
}
