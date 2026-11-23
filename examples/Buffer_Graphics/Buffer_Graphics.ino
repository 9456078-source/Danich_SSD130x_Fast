#include <microWire.h>
#include <Danich_SSD130x_Fast.h>

// Включаем буферный режим (1024 байта ОЗУ):
Danich_SSD130x_Fast<OLED_BUFFER> myOLED(0x3C);

void setup() {
  myOLED.begin(OLED_1309);   // Для экранов на SSD1306 укажите OLED_1306
  Wire.setClock(600000L);    // Разгон I2C до 600 кГц

  myOLED.setFont(SmallFontRus);

  // 1. Рисуем сложную векторную сцену в буфере ОЗУ (дисплей пока не трогаем)
  myOLED.drawRect(0, 0, 128, 64);                 // Сплошная рамка по периметру

  // Наклонные линии (алгоритм Брезенхэма)
  myOLED.drawLine(4, 4, 34, 34);
  myOLED.drawLine(34, 4, 4, 34);

  // Окружности: контур и заливка
  myOLED.drawCircle(64, 19, 15, false);           // Контур круга
  myOLED.drawCircle(64, 19, 7, true);             // Закрашенный круг внутри

  // Закрашенный прямоугольник + инверсный текст поверх него (попиксельно Y=15)
  myOLED.fillRect(88, 6, 34, 26, true);
  myOLED.invText(true);
  myOLED.print(F("1KB"), 96, 15);                 // Координата Y задаётся в пикселях!
  myOLED.invText(false);

  // Разделитель и статическая подпись внизу (Y=46 пикс.)
  myOLED.drawHLine(4, 39, 120, 4, 2);
  myOLED.print(F("Dirty FPS:"), 8, 46);

  // Отправляем всю собранную сцену на экран одной транзакцией
  myOLED.update();
}

void loop() {
  static uint16_t counter = 0;
  static uint32_t tStep = 0;

  if (millis() - tStep >= 50) {
    tStep = millis();

    // Очищаем только прямоугольник под счётчиком и пишем новое число
    myOLED.fillRect(74, 45, 46, 10, false);
    myOLED.printPadZero(counter++, 4, 76, 46);

    // Умный update() сам отправит по I2C ТОЛЬКО изменённый прямоугольник (74..119),
    // не перерисовывая остальной экран!
    myOLED.update();
  }
}