//********************************************************************
//*
//*   Tablo na WS2812b
//*   2021.02.01 - 2021.08.16
//*   v.103
//*
//********************************************************************
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <WiFiManager.h>

const uint32_t utcOffsetInSeconds = 10800;
const uint32_t utcPeriodMseconds = 604800000; //  86400000-р/сут; 604800000-р/нед
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, utcPeriodMseconds);

const uint32_t PIXEL_PIN = 2;

const uint32_t PIXEL_COUNT = 110; //  210
const uint32_t RAZR_PIXEL = 22;   //  42

#define BRIGHT_DAY 150
#define BRIGHT_NIGHT 50
#define TIME_DAY 8
#define TIME_NIGHT 19

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

String z = "";               //  строка данных для отображения
const String strIn = "ww0";  //  начало посылки с весов
const String strOut = " kg"; //  конец посылки с весов
const uint8_t offSet = 3;    //  смещение данных в посылке с весов
const uint8_t digit = 5;     //  кол-во разрядов табло
const uint8_t vesStab = 10;  //  стабильный вес повторение

const uint16_t pauseVisible = 500;
uint32_t pauseVisibleOld = 0;

const uint8_t brightTimeMin = 19; //  время после которого мин яркость
const uint8_t brightTimeMax = 7;  //  время после которого макс яркость
const uint8_t brightMin = 10;
const uint8_t brightMax = 50;

const uint32_t reds = 16711680;      //  цвет НЕ стаб (красный)
const uint32_t greens = 65280;       //  цвет стаб (зеленый)
const uint32_t colorTime = 16766720; //  цвет часы (желтый)
const uint32_t colorTemp = 16711935; //  цвет температура (розовый)

uint16_t strOld = 0;
uint8_t count = 0;
uint32_t color = 11184810;

uint32_t timeReadOld = 0;
bool flagSec = true;
bool flagTime = true;
bool flagMistake = false;
bool flagShow = true;
uint32_t timeOld = 0;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient espClient;

//********************************************************************
//   Чтение данных с COM порта
void serialWork()
{
  uint16_t str = Serial.readStringUntil('\n').substring(offSet, offSet + 5).toInt();

  if (strOld == str)
  {
    count++;
  }
  else
  {
    count = 0;
  }

  if (str < 10)
  {
    z = "DDDD";
    z += str;
  }
  else if (str < 100)
  {
    z = "DDD";
    z += str;
  }
  else if (str < 1000)
  {
    z = "DD";
    z += str;
  }
  else if (str < 10000)
  {
    z = "D";
    z += str;
  }
  else
  {
    z = "";
    z += str;
  }

  if (count >= vesStab && strOld > 50)
  {
    color = greens;
  }
  else
  {
    color = reds;
  }
  strOld = str;
}

//********************************************************************
//  табло 5 x 21 = 110
void visibleWork(String visData, uint32_t color)
{ //  данные, цвет

  for (uint8_t razr = 0; razr < digit; ++razr)
  {
    uint8_t x;
    if (visData.charAt(razr) == 'A')
    {
      x = 10;
    }
    else if (visData.charAt(razr) == 'B')
    {
      x = 11;
    }
    else if (visData.charAt(razr) == 'C')
    {
      x = 12;
    }
    else if (visData.charAt(razr) == 'D')
    {
      x = 13;
    }
    else if (visData.charAt(razr) == 'E')
    {
      x = 14;
    }
    else
    {
      x = visData.substring(razr, razr + 1).toInt();
    }
    switch (x)
    {
    case 0:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if (i >= 0 && i < 20)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 1:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if (i > 4 && i < 13)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 2:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if ((i > 1 && i < 9) || (i > 11 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 3:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if ((i > 1 && i < 16) || (i == 20 || i == 21))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 4:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if ((i >= 0 && i < 3) || (i > 4 && i < 13) || (i > 18 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 5:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if ((i >= 0 && i < 6) || (i > 7 && i < 16) || (i > 18 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 6:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if ((i >= 0 && i < 6) || (i > 7 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 7:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if (i > 1 && i < 13)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 8:
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        strip.setPixelColor(i + razr * RAZR_PIXEL, color);
      }
      break;
    case 9:
      for (uint8_t i = 0; i < RAZR_PIXEL; i++)
      {
        if ((i >= 0 && i < 16) || (i > 18 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 10: //  градус ( A )
      for (uint8_t i = 0; i < RAZR_PIXEL; i++)
      {
        if (i == 0 || i == 1 || i == 3 || i == 4 || i == 6 || i == 7 ||
            i == 20 || i == 21)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 11: //  минус ( B )
      for (uint8_t i = 0; i < RAZR_PIXEL; i++)
      {
        if (i == 8 || (i > 18 && i < 22))
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 12: //  двоеточие ( C )
      for (uint8_t i = 0; i < RAZR_PIXEL; i++)
      {
        if (i == 20 || i == 21)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    case 13: //  null (пусто) ( D )
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
      }
      break;
    case 14: //  точка ( E )
      for (uint8_t i = 0; i < RAZR_PIXEL; ++i)
      {
        if (i == 14)
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, color);
        }
        else
        {
          strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
        }
      }
      break;
    }
  }
}

//********************************************************************
//  табло 5 x 42 = 210
// void visibleWork(String visData, uint32_t color)
// { //  данные, цвет
//   for (uint8_t razr = 0; razr < digit; ++razr)
//   {
//     uint8_t x;
//     if (visData.charAt(razr) == 'A')
//     {
//       x = 10;
//     }
//     else if (visData.charAt(razr) == 'B')
//     {
//       x = 11;
//     }
//     else if (visData.charAt(razr) == 'C')
//     {
//       x = 12;
//     }
//     else if (visData.charAt(razr) == 'D')
//     {
//       x = 13;
//     }
//     else if (visData.charAt(razr) == 'E')
//     {
//       x = 14;
//     }
//     else
//     {
//       x = visData.substring(razr, razr + 1).toInt();
//     }
//     switch (x)
//     {
//     case 0:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if (i >= 0 && i < 36)
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 1:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if (i > 11 && i < 24)
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 2:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i > 5 && i < 18) || (i > 23 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 3:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i > 5 && i < 30) || (i > 36 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 4:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i >= 0 && i < 6) || (i > 11 && i < 24) || (i > 35 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 5:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i >= 0 && i < 12) || (i > 17 && i < 30) || (i > 35 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 6:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i >= 0 && i < 12) || (i > 17 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 7:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if (i > 5 && i < 24)
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 8:
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//       }
//       break;
//     case 9:
//       for (uint16_t i = 0; i < RAZR_PIXEL; i++)
//       {
//         if ((i >= 0 && i < 30) || (i > 35 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 10: //  градус ( A )
//       for (uint16_t i = 0; i < RAZR_PIXEL; i++)
//       {
//         if ((i >= 0 && i < 18) || (i > 35 && i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 11: //  минус ( B )
//       for (uint16_t i = 0; i < RAZR_PIXEL; i++)
//       {
//         if ((i > 35) && (i < 42))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 12: //  двоеточие ( C )
//       for (uint16_t i = 0; i < RAZR_PIXEL; i++)
//       {
//         if ((i == 36) || (i == 37) || (i == 40) || (i == 41))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     case 13: //  null (пусто) ( D )
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//       }
//       break;
//     case 14: //  точка ( E )
//       for (uint16_t i = 0; i < RAZR_PIXEL; ++i)
//       {
//         if ((i == 26) || (i == 27))
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, color);
//         }
//         else
//         {
//           strip.setPixelColor(i + razr * RAZR_PIXEL, strip.Color(0, 0, 0));
//         }
//       }
//       break;
//     }
//   }
// }

//*****************************************************************************
//  время из Internet
void TimeNTP()
{
  color = colorTime;

  if (timeClient.getHours() < TIME_NIGHT && timeClient.getHours() >= TIME_DAY)
  {
    strip.setBrightness(BRIGHT_DAY);
  }
  else
  {
    strip.setBrightness(BRIGHT_NIGHT);
  }

  z = "";
  if (timeClient.getHours() < 10)
  {
    z += "D";
  }
  z += timeClient.getHours(); //  часы
  if (flagSec)
  {
    z += "C";
  }
  else
  {
    z += "D";
  }
  if (timeClient.getMinutes() < 10)
  {
    z += "0";
  }
  z += timeClient.getMinutes(); //  минуты
}

//********************************************************************
//  визуальные эффекты
void show()
{
  //  очистка табло
  for (uint8_t i = 0; i < PIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();

  uint8_t pause;
  if (RAZR_PIXEL > 150)
  {
    pause = 10;
  }
  else
  {
    pause = 20;
  }
  //  эффект зажигание
  for (uint8_t i = 0; i < PIXEL_COUNT / 2; ++i)
  {
    if (Serial.available())
      break;
    strip.setPixelColor(i, strip.Color(random(1, 255), random(1, 255), random(1, 255)));
    strip.setPixelColor(PIXEL_COUNT - i - 1, strip.Color(random(1, 255), random(1, 255), random(1, 255)));
    strip.show();
    delay(pause);
  }
  delay(500);

  //  эффект угасания
  for (uint8_t i = 0; i < PIXEL_COUNT / 2; ++i)
  {
    if (Serial.available())
      break;
    strip.setPixelColor(i, 0, 0, 0);
    strip.setPixelColor(PIXEL_COUNT - i - 1, 0, 0, 0);
    strip.show();
    delay(pause);
  }
  delay(500);
  z = "DDDDD";
}

//*****************************************************************************
void setup()
{
  Serial.begin(9600);
  Serial.swap();

  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHT_DAY);

  visibleWork("ABABA", colorTemp);
  strip.show();

  WiFiManager wifiManager;
  wifiManager.autoConnect("tablo_AP");

  z = WiFi.localIP().toString();
  z = z.substring(z.lastIndexOf('.') + 1, z.length());
  visibleWork("DE" + z, colorTemp);
  strip.show();
  delay(2000);

  timeClient.begin();

  while (!timeClient.update())
  {
    delay(500);
    z = "BBBBB";
    visibleWork(z, colorTemp);
    strip.show();
  }

  httpServer.begin();
  httpUpdater.setup(&httpServer);

  timeOld = millis();
  color = colorTime;
}
//********************************************************************
void loop()
{
  httpServer.handleClient();

  if (Serial.available())
  {
    serialWork();
    pauseVisibleOld = millis();
    flagShow = true;
  }

  if ((millis() - pauseVisibleOld) >= pauseVisible)
  {
    if (flagShow)
    {
      show();
      flagShow = false;
    }
    TimeNTP();
  }

  visibleWork(z, color);
  strip.show();

  if (millis() - timeReadOld >= 500)
  {
    flagSec = !flagSec;
    timeReadOld = millis();
  }
}

//********************************************************************