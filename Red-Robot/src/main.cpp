#include <Arduino.h>

int latchPin = 12;
int clockPin = 11;
int dataPin = 14;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

int i = 0;

void shiftshift(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
  uint8_t i;

  for (i = 0; i < 8; i++)
  {
    if (bitOrder == LSBFIRST)
      digitalWrite(dataPin, !!(val & (1 << i)));
    else
      digitalWrite(dataPin, !!(val & (1 << (7 - i))));

    delayMicroseconds(10);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(clockPin, LOW);
  }
}

void loop()
{
  // put your main code here, to run repeatedly:

  Serial.printf("jkl %d\n", i++);

  digitalWrite(latchPin, LOW);
  shiftshift(dataPin, clockPin, MSBFIRST, 1);
  digitalWrite(latchPin, HIGH);
  delay(250);
}
