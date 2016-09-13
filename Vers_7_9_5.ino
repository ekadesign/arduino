// ИСПОЛЬЗОВАНИЕ SD ДЛЯ СОХРАНЕНИЯ ПРОМЕЖУТОЧНЫХ ДАННЫХ (V7_9_5)
// (отправка данных о литрах, суммах и сумме инкассации в любом случае)
// Датчик температуры - НЕТ !!!!!!!!!!!!
// Пополнение баланса клиента с TM после налива воды
// print -> write
// timeSec15 = 20000
// msg[1100]
// Убрал блокирование монето/купюроприемников
// Зацикливание variant=1 добавление в него обработки денег
// Без обновления сообщений на ЖК-монитор
// Добавление в запросы суммы копилок
// Новый алгоритм сохранения данных
// Прием денег во время налива
// Отправка сведений о состоянии аппарата каждые 20 минут
// ============================================================
// Выводы 50, 51 и 52 (Arduino Mega)
// Вывод  53 для SS
// ============================================================

#include <EEPROM.h>
#include <OneWire.h>
#include <SIM900.h>
#include <SoftwareSerial.h>
#include <inetGSM.h>
#include <LiquidCrystalRus.h>
#include <SD.h>
#include <SPI.h>

//============= Для обмена с сервером ==========================================================
InetGSM inet;

File fCoin;      // Монеты
File fBanknote;  // Банкноты
File fNNNN;      // Номер запроса

char msg[1100];
char abc;
int numdata = 0;
int i = 0;
int upd = 0;                                   // Флаг отправки
String stroka = "";                            // Строка для отправки данных на сервер
const String stroka0 = "/dashboard/api.php?";  // Путь к серверу
String stroka1 = "";                           // Ключ пользователя для отправки данных на сервер
const String P1 = "&sw=oVXq74Z0Y1";            // Пароль при запросе данных о ключе
const String P2 = "&sw=xeoANrSbdR";            // Пароль при запросе об обновлении баланса
const String P3 = "&sw=lQBMcWhO6m";            // Пароль при запросе цены на литр воды
const String P4 = "&sw=AHA1GTPP2z";            // Пароль при запросе о сумме инкассации
const String P5 = "&sw=qq5BjoKwxx";            // Пароль при запросе о работе с наличкой
const String serverIP = "81.4.243.112";        // IP сервера
//==============================================================================================

const String numApp = "13";    // Номер аппарата по разливу воды

OneWire ds(24);                // Порт TM
OneWire dtemp(28);             // Порт датчика температуры DS18B20
const byte banknotePin = 21;   // Порт купюроприемника
const byte coinPin = 20;       // Порт монетоприемника
const byte puskPin = 23;       // Порт кнопки "Пуск"
const byte udarPin = 25;       // Порт датчика УДАР
// const byte blokbanknotePin = 26;    // Порт блокировки купюроприемника
// const byte blokcoinPin = 27;        // Порт блокировки монетоприемника
const byte dverPin = 29;        // Порт состояния дверцы аппарата
const byte relay1Pin  = 30;     // Порт реле насоса подачи воды
const byte relay2Pin  = 32;     // Порт реле выключения монето/купюроприемника
const byte relay3Pin  = 34;     // Порт реле вентилятора
const byte zamokPin   = 36;     // Порт реле замка
const byte relay4Pin  = 44;     // Порт реле 220 V
const byte flowPin  = 2;        // Порт датчика воды
// const byte flowPin  = 3;     // Порт датчика воды
const byte nowaterPin  = 46;    // Порт нижнего датчика воды  (Нет воды)
const byte littlewaterPin = 47; // Порт среднего датчика воды (Мало воды)
const byte fulltankPin  = 48;   // Порт верхнего датчика воды (Полный бак)
const byte DSPin = 53;          // Порт DS

int cenaLitr = 3;               // Цена 1 литра воды - минимум
long coinItog = 0;              // Сумма монет
// long coinItogOld = 0;           // Сумма монет до перезагрузки
long banknoteItog = 0;          // Сумма банкнот
// long banknoteItogOld = 0;       // Сумма банкнот до перезагрузки
long NNNN = 0;                  // Номер запроса

int HighByte;
int LowByte;
int TReading;
byte datad[12];
int T     = 0;

const String pathClient = "Client/";  // Путь к папке клиентов
const String pathInkass = "Inkass/";  // Путь к папке инкассаторов
String hh = "";
String bb = "";                 // Строка для значения баланса или суммы инкассации
String co = "";                 // Строка для значения coinItog
String ba = "";                 // Строка для значения banknoteItog
String ID = "";                 // Строка идентификатора владельца TM
String kluchPolzov;             // Ключ клиента или инкассатора
String nameClient;              // Имя файла клиента
String nameInkass;              // Имя файла инкассатора
byte addr[8];                   // Массив байт считываемого ключа TM
byte addrT[8];                  // Массив байт датчика температуры
byte inkass[8];                 // Массив байт считываемого ключа инкассатора
byte kluchEEPROM[8];            // Массив байт главного ключа TM на Arduino

int kolich = 0;                 // kolich=avans/cenaLitr - литры
int kvota = 0;                  // Порция налива воды по кнопке ПУСК
int pulseKvota = 0;             // Число импульсов для налива воды
volatile int variant = 0;       // 0 - приглашение Аванс/TM

int variant1 = 1;
volatile int variant2 = 0;
// boolean kopilka = true;                // true - один раз проверка копилок
boolean kkk = true;                    // true - аванс или false - ключ
boolean kkk1 = true;
boolean kkk2 = true;
boolean otvetOtpr = false;             // true - успешная отправка, false - неудача
boolean kluchClient = false;
boolean priznClient = false;           // true - наличие файла в папке Client
boolean priznInkass = false;           // true - наличие файла в папке Inkass

int kluch = 0;                         // 0 - таблетка не действует
// 1 - ключ администратора
// 2 - ключ покупателя (balans)
// 3 - ключ инкассатора
// 4 - нет связи с сервером

int clickPusk = 0;                     // 0-кнопка отжата, 1-кнопка нажата
volatile int pulse_banknote = 0;       // Счетчик импульсов купюроприемника
volatile int pulse_coin = 0;           // Счетчик импульсов монетоприемника
volatile int pulse_flow = 0;           // Счетчик импульсов датчика воды
int pulseLiter = 0;                    // Число импульсов на 1 литр воды
int tempKrit = -3;                     // Критическая температура

volatile unsigned long time_start = 0; // Время начала отсчета для сброса
// unsigned long time_banknote = 0;       // Время первого импульса купюры
// unsigned long time_coin = 0;           // Время первого импульса монеты
unsigned long time_inkass = 0;         // Начальное время отсчета действий инкассатора
unsigned long beginPusk = 0;           // Начальное время нажатия кнопки ПУСК
// unsigned long time_kopilka = 0;        // Время начала отсчета копилки
unsigned long beginOtpravki = 0;       // Начальное время отсчета до передачи сведений

// int nominal[21] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}; // Цены

// const int nominal[11] = {0, 1, 2, 0, 0, 5, 10, 10, 10, 10, 10}; // OLD
// int nominal[11] = {0, 1, 1, 2, 2, 5, 5, 10, 10, 10, 10}; // Цены монет в рублях
// int nominal[11] = {0, 1, 2, 0, 0, 5, 0, 0, 0, 0, 10}; // Цены монет в рублях
//int nominal[7] = {0, 1, 2, 0, 0, 5, 10}; // Цены монет в рублях

// float litrPulse = 0.00135;            // 1 импульс = 0.027 л.
const float litrPulse = 0.026;           // 1 импульс = 0.027 л.
// volatile long avans  = 0;                // Аванс
volatile long avansClient = 0;           // Внесенный аванс любого клиента
long literClient = 0;                    // Налитая вода в литрах
volatile long balans = 0;                // Текущий баланс владельца ключа
long inkasso = 0;                        // Сумма предыдущей инкассации
long itogo  = 0;                         // Итоговая наличность
// long clientAvans = 0;                    // Запоминание внесенной налички
// long clientLiter = 0;                    // Запоминание налитой воды

LiquidCrystalRus lcd(4, 5, 10, 11, 12, 13); // RS=4, E=5
// D7=13, D6=12, D5=11, D4=10

void setup()
{
  int z = 0;
  boolean started = false;
  boolean flagSD;                        // true успешная связь с SD
  boolean flagok = true;

  pinMode(banknotePin, INPUT);           // Порт купюроприемника            - ВХОД
  pinMode(coinPin, INPUT);               // Порт монетоприемника            - ВХОД
  pinMode(flowPin, INPUT);               // Порт датчика воды               - ВХОД
  pinMode(fulltankPin, INPUT);           // Порт датчика "Полный бак"       - ВХОД
  pinMode(littlewaterPin, INPUT);        // Порт датчика "Мало воды"        - ВХОД
  pinMode(nowaterPin, INPUT);            // Порт датчика "Нет воды"         - ВХОД
  //  pinMode(blokbanknotePin, OUTPUT);    // Порт блокировки купюроприемника - ВЫХОД
  //  digitalWrite(blokbanknotePin, LOW);  // Разблокировка купюроприемника
  //  pinMode(blokcoinPin, OUTPUT);        // Порт блокировки монетоприемника - ВЫХОД
  //  digitalWrite(blokcoinPin, LOW);      // Разблокировка монетоприемника
  pinMode(puskPin, INPUT);               // Порт кнопки ПУСК                - ВХОД
  pinMode(dverPin, INPUT);               // Порт состояния дверцы аппарата  - ВХОД
  pinMode(zamokPin, OUTPUT);             // Порт замка дверцы               - ВЫХОД
  digitalWrite(zamokPin, HIGH);          // Замок дверцы закрыт
  pinMode(udarPin, INPUT);               // Порт датчика УДАР               - ВХОД
  pinMode(relay1Pin, OUTPUT);            // Порт реле насоса подачи воды    - ВЫХОД
  digitalWrite(relay1Pin, LOW);          // Реле разомкнуто
  pinMode(relay2Pin, OUTPUT);            // Порт реле выключения "денег"    - ВЫХОД
  digitalWrite(relay2Pin, HIGH);         // Реле разомкнуто
  pinMode(relay3Pin, OUTPUT);            // Порт реле вентилятора           - ВЫХОД
  digitalWrite(relay3Pin, HIGH);         // Реле разомкнуто
  pinMode(relay4Pin, OUTPUT);            // Порт реле 220 V                 - ВЫХОД
  digitalWrite(relay4Pin, HIGH);         // Реле разомкнуто
  pinMode(DSPin, OUTPUT);                // Порт DS                         - ВЫХОД

  Serial.begin(9600);
  //  Serial.begin(57600);

  pulseLiter = 1 / litrPulse;
  Serial.print("pL=");
  Serial.println(pulseLiter);

  lcd.begin(16, 2);                      // 16 символов в двух строках ЖК-монитора

  while (true)
  {
    flagSD = SD.begin(53);
    if (!flagSD)                              // Нет SD
    {
      if (flagok)
      {
        Serial.println("W01");
        flagok = false;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("     Аппарат    ");
      lcd.setCursor(0, 1);
      lcd.print("  не работает!  ");
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    Проблема    ");
      lcd.setCursor(0, 1);
      lcd.print("  с SD-картой!  ");
      delay(3000);
    }
    else
    {
      Serial.println("W02");
      break;
    }
  }

  lcd.clear();                              // Очистка экрана ЖК-монитора
  lcd.setCursor(0, 0);
  lcd.print("   Настройка    ");
  lcd.setCursor(0, 1);
  lcd.print(" GPRS-соединения");

  //============= Настройка соединения с сервером =====================
  Serial.println("W03");
  if (gsm.begin(9600))
  {
    Serial.println("\W04");
    started = true;
  }
  else Serial.println("\W17");

  //             Главный ключ     Ключ инкассатора     Ключ клиента
  //             ------------     ----------------     ------------
  // addr[0]  =   0x01;    1         0x01;    1         0x01;    1
  // addr[1]  =   0x3B;   59         0x43;   67         0x37;   55
  // addr[2]  =   0x7C;  124         0x1A;   26         0x67;  103
  // addr[3]  =   0x4A;   74         0xA9;  169         0xF4;  244
  // addr[4]  =   0x00;    0         0x00;    0         0x00;    0
  // addr[5]  =   0x0D;   13         0x00;    0         0x00;    0
  // addr[6]  =   0x0A;   10         0x00;    0         0x00;    0
  // addr[7]  =   0x68;  104         0xF2;  234         0x6A;  106

  //  for (int i = 0; i < 8; i++) EEPROM.write(i, addr[i]);

  if (started)
  {
    //    if (inet.attachGPRS("internet", "", ""))
    if (inet.attachGPRS("internet", "mts", "mts"))
      Serial.println("W05");
    else Serial.println("W06");
    delay(1000);
  }

  //============= Начальная работа с SD ===============================

  ba = "";
  co = "";

  if (SD.exists("Cena1L.txt"))                      // Существует файл NNNN.txt
  {
    File fCena1L = SD.open("Cena1L.txt", FILE_WRITE);
    fCena1L.seek(0);
    co = "";
    while (fCena1L.peek() != 0x0D)
    {
      abc = fCena1L.read();
      co += abc;
    }
    fCena1L.close();                                // Закрытие файла
  }
  else                                              // Не существует файл NNNN.txt
  {
    File fCena1L = SD.open("Cena1L.txt", FILE_WRITE);
    fCena1L.println(3);                             // Запись 3 руб. за 1 литр
    fCena1L.flush();
    fCena1L.close();                                // Закрытие файла
    co = "3";
  }
  cenaLitr = co.toInt();                            // Цена 1 литра (руб.)

  if (SD.exists("NNNN.txt"))                        // Существует файл NNNN.txt
  {
    fNNNN = SD.open("NNNN.txt", FILE_WRITE);
    fNNNN.seek(0);
    co = "";
    while (fNNNN.peek() != 0x0D)
    {
      abc = fNNNN.read();
      co += abc;
    }
  }
  else                                              // Не существует файл NNNN.txt
  {
    fNNNN = SD.open("NNNN.txt", FILE_WRITE);
    fNNNN.println(0);                               // Запись нуля
    fNNNN.flush();
    co = "0";
  }
  NNNN = co.toInt();                                // Последний номер запроса

  if (SD.exists("Coin.txt"))                        // Существует файл Coin.txt
  {
    fCoin = SD.open("Coin.txt", FILE_WRITE);
    fCoin.seek(0);                                  // На начало файла
    co = "";
    while (fCoin.peek() != 0x0D)
    {
      abc = fCoin.read();
      co += abc;
    }
  }
  else                                              // Не существует файл Coin.txt
  {
    fCoin = SD.open("Coin.txt", FILE_WRITE);
    fCoin.println(0);                               // Запись нуля
    fCoin.flush();
    co = "0";
  }
  coinItog = co.toInt();                            // Текущая сумма монет
  //  coinItogOld = coinItog;                           // Сумма монет до перезагрузки

  if (SD.exists("Banknote.txt"))                    // Существует файл Banknote.txt
  {
    fBanknote = SD.open("Banknote.txt", FILE_WRITE);
    fBanknote.seek(0);                              // На начало файла
    ba = "";
    while (fBanknote.peek() != 0x0D)
    {
      abc = fBanknote.read();
      ba += abc;
    }
  }
  else                                              // Не существует файл Banknote.txt
  {
    fBanknote = SD.open("Banknote.txt", FILE_WRITE);
    fBanknote.println(0);                           // Запись нуля
    fBanknote.flush();
    ba = "0";
  }
  banknoteItog = ba.toInt();                        // Текущая сумма банкнот
  //  banknoteItogOld = banknoteItog;               // Сумма банкнот до перезагрузки

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Монеты:" + co + " р.");
  lcd.setCursor(0, 1);
  lcd.print("Купюры:" + ba + " р.");
  Serial.println("W07=" + co + " W08=" + ba);
  delay(2000);

  if (!SD.exists("Client")) SD.mkdir("Client");     // Создание папки Client
  if (!SD.exists("Inkass")) SD.mkdir("Inkass");     // Создание папки Inkass

  //============= Конец настройки SD ==================================

  /*
    // Проверка наличия датчика температуры
    if (!dtemp.search(addrT))
    {
      Serial.print("Temperature:   No more addresses.\n");
      dtemp.reset_search();
      return;
    }
    if (OneWire::crc8( addrT, 7) != addrT[7])
    {
      Serial.print("Temperature:   CRC is not valid!\n");
      return;
    }
    if (addrT[0] != 0x28)
    {
      Serial.print("Temperature:   Device is not a DS18B20 family device.\n");
      return;
    }
  */

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Идет настройка.");
  lcd.setCursor(0, 1);
  lcd.print("  Подождите ... ");
  Serial.println("");
  Serial.println("W09");
  Serial.print("W10=");
  for (int i = 0; i < 8; i++)           // Чтение 8 байт ключа главного админа
  {
    kluchEEPROM[i] = EEPROM.read(i);
    Serial.print(kluchEEPROM[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  //============= Внешние прерывания ==================================
  //  attachInterrupt(0, count_flow, RISING);    // LOW  -> HIGH
  //  attachInterrupt(1, count_flow, RISING);    // LOW  -> HIGH
  attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
  attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW
  //===================================================================

  //---------------- Отправка сведений на сервер --------------------------
  for (int i = 0; i < 5; i++)
  {
    Serial.println("Pop=" + String(i + 1));
    int WS    = 3;               // 0-нет воды, 1-мало воды, 2-полный бак, 3-нормально
    int Dv    = 0;               // Дверца
    int NP    = 0;               // 0-нет 220, 1-есть 220
    int MD    = 0;               // 0-нет удара, 1-удар
    String FW = "V7_9_5";

    // Отправка на сервер состояния датчиков и количестве денег
    if (!digitalRead(nowaterPin))          // Нет воды
    {
      WS = 0;
    }
    else if (digitalRead(fulltankPin))     // Полный бак
    {
      WS = 2;
    }
    else if (!digitalRead(littlewaterPin)) // Мало воды
    {
      WS = 1;
    }

    if (digitalRead(dverPin))              // Дверца закрыта
      //    if (!digitalRead(dverPin))              // Дверца закрыта
    {
      Dv = 1;
    }
    else                                   // Дверца открыта
    {
      Dv = 0;
    }

    /*
        dtemp.reset();
        dtemp.select(addrT);
        dtemp.write(0x44,1); // запускаем конвертацию
        delay(750); // ждем 750ms
        dtemp.reset();
        dtemp.select(addrT);
        dtemp.write(0xBE); // считываем ОЗУ датчика
        for (byte i = 0; i < 9; i++)
        { // обрабатываем 9 байт
          datad[i] = dtemp.read();
          Serial.print(datad[i], HEX);
          Serial.print(" ");
        }
        // высчитываем температуру :)
        LowByte = datad[0];
        Serial.print("LB=");
        Serial.print(LowByte,HEX);
        HighByte = datad[1];
        Serial.print("   HB=");
        Serial.print(HighByte,HEX);
        T = ((HighByte << 8)+LowByte)/2;        // Температура
    */
    T = 25;

    Serial.println(" T=" + String(T));

    if (digitalRead(relay4Pin))             // Есть 220 V
    {
      NP = 1;
    }
    else                                    // Нет 220 V
    {
      NP = 0;
    }
    if (digitalRead(udarPin))               // Удар
    {
      MD = 1;
    }
    else
    {
      MD = 0;
    }

    itogo = coinItog + banknoteItog;

    stroka = stroka0 + P3 + "&U=" + numApp + "&m=";
    stroka += String(itogo) + "&ws=" + String(WS) + "&t=" + String(T);
    stroka += "&np=" + String(NP) + "&fw=" + String(FW) + "&md=" + String(MD) + "&dstatus=" + String(Dv);

    Serial.println("W14=" + stroka);

    memset(msg, '\0', 1100);
    numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

    stroka = "";
    z = strlen(msg);
    for (int i = z - 30; i < numdata; i++) stroka += msg[i];

    Serial.println("\nW15: " + String(numdata));
    Serial.println("\nW16:");
    Serial.println("<" + stroka + ">");

    if (numdata != 0)                   // Есть ответ
    {
      // Разбор ответного сообщения и присвоение значения
      int j;
      int m;
      int q;
      int cena1Litra;
      j = stroka.indexOf("W:");
      m = stroka.indexOf("T:");
      q = stroka.indexOf("_(]AdIXW");
      if (q >= 0)
      {
        if (j >= 0)
        {
          cena1Litra = (stroka.substring(j + 2, m - 1)).toInt();
          if (cena1Litra != cenaLitr)
          {
            cenaLitr = cena1Litra;
            File fCena1L = SD.open("Cena1L.txt", FILE_WRITE);
            fCena1L.seek(0);
            fCena1L.println(cenaLitr);         // Запись 3 руб. за 1 литр
            fCena1L.flush();
            fCena1L.close();                   // Закрытие файла
          }
          Serial.println("cL=" + String(cenaLitr));

          if (m > 0)
          {
            tempKrit = (stroka.substring(m + 2, q)).toInt();
            Serial.println("tKrit=" + String(tempKrit));
          }
          break;
        }
      }
      delay(5000);
    }
  }

  delay(5000);

  // Отправка на сервер данных
  rabotaInkass();
  delay(2000);
  rabotaNalich();
  delay(2000);
  rabotaClient();
  delay(2000);

  Serial.print("dver=" + String(digitalRead(dverPin)));
  Serial.print(" Nas=" + String(digitalRead(relay1Pin)));
  Serial.print(" Mon=" + String(digitalRead(relay2Pin)));
  Serial.print(" Vent=" + String(digitalRead(relay3Pin)));
  Serial.println(" Zam=" + String(digitalRead(zamokPin)));
  Serial.print("no_wat=" + String(digitalRead(nowaterPin)));
  Serial.print(" malo=" + String(digitalRead(littlewaterPin)));
  Serial.print(" polno=" + String(digitalRead(fulltankPin)));
  Serial.print(" puskP=" + String(digitalRead(puskPin)));
  Serial.print(" coinP=" + String(digitalRead(coinPin)));
  Serial.println(" bankP=" + String(digitalRead(banknotePin)));

  Serial.println("NN=" + String(NNNN));

  beginOtpravki = millis();            // Начало отсчета времени до отправки на сервер
  //  time_kopilka  = beginOtpravki;       // Начало отсчета копилки

  Serial.println(String(millis()));
}

void (* resetFunc) (void) = 0;

void loop()
{
  //  return;
  int z = 0;
  unsigned long timeSbrosa = 60000;      // Время для сброса аванса  (1 мин.)
  unsigned long timeOtpravki = 1200000;  // Время для отправки сведений на сервер (10 мин.)
  unsigned long timeAllBanknote = 3000;  // Макс.время приема купюры (3 сек.)
  unsigned long timeAllCoin = 1400;      // Макс.время приема монеты (2 сек.)
  unsigned long timeSec15 = 20000;       // Время ожидания действий владельца TM (20 сек.)
  unsigned long timeInkass = 30000;      // Время сброса данных для инкассатора

  if ((variant == 0) && ((pulse_coin + pulse_banknote) == 0) && (millis() - beginOtpravki > timeOtpravki))
  {
    int WS    = 3;                       // 0-нет воды, 1-мало воды, 2-полный бак
    int Dv    = 0;                       // Дверца
    int NP    = 0;                       // 0-нет 220, 1-есть 220
    int MD    = 0;                       // 0-нет удара, 1-удар
    int cena1Litra;
    String FW = "V7_9_5";
    lcd.clear();           // Очистка экрана
    lcd.setCursor(0, 0);
    lcd.print("   Подождите    ");
    lcd.setCursor(0, 1);
    lcd.print("   немного ...  ");
    // Отправка на сервер данных папок Client, Inkass и файла Nalich.TXT,
    // показаний датчиков и суммы копилок, только после этого анализ ключа
    rabotaNalich();
    rabotaClient();
    rabotaInkass();
    //Отправка копилки и состояния датчиков на сервер
    /*
        // Блокирование приема денег и использования TM клиентов
        detachInterrupt(2);                          // Выключение обработки прерывания
        detachInterrupt(3);                          // Выключение обработки прерывания
        digitalWrite(relay2Pin, LOW);                // Блокировка приема монет/купюр
    */
    if (!digitalRead(nowaterPin))            // true - Нет воды
    {
      WS = 0;
    }
    else if (digitalRead(fulltankPin))          // Полный бак
    {
      WS = 2;
    }
    else if (!digitalRead(littlewaterPin)) // Мало воды
    {
      WS = 1;
    }

    if (digitalRead(dverPin))              // Дверца закрыта
      //    if (!digitalRead(dverPin))              // Дверца закрыта
    {
      Dv = 1;
    }
    else                                    // Дверца открыта
    {
      Dv = 0;
    }

    /*
          dtemp.reset();
          dtemp.select(addrT);
          dtemp.write(0x44,1); // запускаем конвертацию
          delay(750); // ждем 750ms
          dtemp.reset();
          dtemp.select(addrT);
          dtemp.write(0xBE); // считываем ОЗУ датчика
          for (byte i = 0; i < 9; i++)
          { // обрабатываем 9 байт
            data[i] = dtemp.read();
            Serial.print(data[i], HEX);
            Serial.print(" ");
          }
          // высчитываем температуру :)
          LowByte = data[0];
          Serial.print("LB=");
          Serial.print(LowByte,HEX);
          HighByte = data[1];
          Serial.print("   HB=");
          Serial.print(HighByte,HEX);
          T = ((HighByte << 8)+LowByte)/2;        // Температура
          Serial.print(" T=");Serial.println(T);

    */

    T = 25;

    Serial.print(" T="); Serial.println(T);

    if (digitalRead(relay4Pin))             // true - Есть 220 V
    {
      NP = 1;
    }
    else                                    // false - Нет 220 V
    {
      NP = 0;
    }
    if (digitalRead(udarPin))               // true - Удар
    {
      MD = 1;
    }
    else                                    // false - Удара нет
    {
      MD = 0;
    }
    co = "";
    fCoin.seek(0);
    while (fCoin.peek() != 0x0D)
    {
      abc = fCoin.read();
      co += abc;
    }
    coinItog = co.toInt();
    ba = "";
    fBanknote.seek(0);
    while (fBanknote.peek() != 0x0D)
    {
      abc = fBanknote.read();
      ba += abc;
    }
    banknoteItog = ba.toInt();
    Serial.println("W07=" + String(coinItog) + "W08=" + String(banknoteItog));
    itogo = coinItog + banknoteItog;

    stroka = stroka0 + P3 + "&U=" + numApp + "&m=";
    stroka += String(itogo) + "&ws=" + String(WS) + "&t=" + String(T);
    stroka += "&np=" + String(NP) + "&fw=" + String(FW) + "&md=" + String(MD) + "&dstatus=" + String(Dv);

    Serial.println("W14=" + stroka);

    memset(msg, '\0', 1100);
    numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

    stroka = "";
    z = strlen(msg);
    for (int i = z - 30; i < numdata; i++) stroka += msg[i];

    Serial.println("\nW15: " + String(numdata));
    Serial.println("\nW16:");
    Serial.println("<" + stroka + ">");
    Serial.println(" ");

    if (numdata != 0)                   // Есть ответ
    {
      // Разбор ответного сообщения и присвоение значения
      int j;
      int m;
      int q;
      j = stroka.indexOf("W:");
      m = stroka.indexOf("T:");
      q = stroka.indexOf("_(]AdIXW");
      if (q > 0)
      {
        if (j >= 0)
        {
          cena1Litra = (stroka.substring(j + 2, m - 1)).toInt();
          if (cena1Litra != cenaLitr)
          {
            cenaLitr = cena1Litra;
            File fCena1L = SD.open("Cena1L.txt", FILE_WRITE);
            fCena1L.seek(0);
            fCena1L.println(cenaLitr);         // Запись 3 руб. за 1 литр
            fCena1L.flush();
            fCena1L.close();                   // Закрытие файла
          }
          Serial.println("cL=" + String(cenaLitr));
        }
        if (m > 0)
        {
          tempKrit = (stroka.substring(m + 2, q)).toInt();
          Serial.println("tempKr=" + String(tempKrit));
        }
      }
    }
    beginOtpravki = millis();
    Serial.println(String(millis()));
  }

  //---------------- Обработка касания таблеткой TM -----------------------
  if (variant == 0)
  {
    boolean kluchAdmin  = true;
    kluchClient = false;
    //    byte j;

    kluch = 0;

    //    if (digitalRead(dverPin) && digitalRead(nowaterPin) && (ds.search(addr)))
    if (digitalRead(dverPin) && (ds.search(addr)))     // Дверца закрыта
      //    if (!digitalRead(dverPin) && digitalRead(nowaterPin) && (ds.search(addr)))
    { // Устройство найдено
      Serial.println("W11");

      // проверка CRC
      if ((OneWire::crc8(addr, 7) == addr[7]) && (addr[0] == 0x01))
      { // Проверка CRC и проверка кода устройства
        int WS    = 3;                       // 0-нет воды, 1-мало воды, 2-полный бак
        int Dv    = 0;                       // Дверца
        int NP    = 0;                       // 0-нет 220, 1-есть 220
        int MD    = 0;                       // 0-нет удара, 1-удар
        int cena1Litra;
        String FW = "V7_9_5";

        lcd.clear();           // Очистка экрана
        lcd.setCursor(0, 0);
        lcd.print("Подождите!  Идет");
        lcd.setCursor(0, 1);
        lcd.print(" проверка ключа ");


        //        digitalWrite(blokbanknotePin, HIGH); // Блокировка приема купюр
        //        digitalWrite(blokcoinPin, HIGH);     // Блокировка приема монет

        kluchAdmin = true;
        for (int i = 0; i < 8; i++) // Сравнение ключа с главным ключом
        {
          if (addr[i] != kluchEEPROM[i])
          {
            kluchAdmin = false;
            break;
          }
        }

        ds.reset_search();
        ds.reset();

        // Отправка на сервер данных папок Client, Inkass и файла Nalich.TXT,
        // показаний датчиков и суммы копилок, только после этого анализ ключа
        rabotaNalich();
        rabotaClient();
        rabotaInkass();
        //Отправка копилки и состояния датчиков на сервер
        /*
            // Блокирование приема денег и использования TM клиентов
            detachInterrupt(2);                          // Выключение обработки прерывания
            detachInterrupt(3);                          // Выключение обработки прерывания
            digitalWrite(relay2Pin, LOW);                // Блокировка приема монет/купюр
        */
        if (!digitalRead(nowaterPin))            // true - Нет воды
        {
          WS = 0;
        }
        else if (digitalRead(fulltankPin))          // Полный бак
        {
          WS = 2;
        }
        else if (!digitalRead(littlewaterPin)) // Мало воды
        {
          WS = 1;
        }

        if (digitalRead(dverPin))              // Дверца закрыта
          //    if (!digitalRead(dverPin))              // Дверца закрыта
        {
          Dv = 1;
        }
        else                                    // Дверца открыта
        {
          Dv = 0;
        }

        /*
              dtemp.reset();
              dtemp.select(addrT);
              dtemp.write(0x44,1); // запускаем конвертацию
              delay(750); // ждем 750ms
              dtemp.reset();
              dtemp.select(addrT);
              dtemp.write(0xBE); // считываем ОЗУ датчика
              for (byte i = 0; i < 9; i++)
              { // обрабатываем 9 байт
                data[i] = dtemp.read();
                Serial.print(data[i], HEX);
                Serial.print(" ");
              }
              // высчитываем температуру :)
              LowByte = data[0];
              Serial.print("LB=");
              Serial.print(LowByte,HEX);
              HighByte = data[1];
              Serial.print("   HB=");
              Serial.print(HighByte,HEX);
              T = ((HighByte << 8)+LowByte)/2;        // Температура
              Serial.print(" T=");Serial.println(T);

        */

        T = 25;

        Serial.print(" T="); Serial.println(T);

        if (digitalRead(relay4Pin))             // true - Есть 220 V
        {
          NP = 1;
        }
        else                                    // false - Нет 220 V
        {
          NP = 0;
        }
        if (digitalRead(udarPin))               // true - Удар
        {
          MD = 1;
        }
        else                                    // false - Удара нет
        {
          MD = 0;
        }
        co = "";
        fCoin.seek(0);
        while (fCoin.peek() != 0x0D)
        {
          abc = fCoin.read();
          co += abc;
        }
        coinItog = co.toInt();
        ba = "";
        fBanknote.seek(0);
        while (fBanknote.peek() != 0x0D)
        {
          abc = fBanknote.read();
          ba += abc;
        }
        banknoteItog = ba.toInt();
        Serial.println("W07=" + String(coinItog) + "W08=" + String(banknoteItog));
        itogo = coinItog + banknoteItog;

        stroka = stroka0 + P3 + "&U=" + numApp + "&m=";
        stroka += String(itogo) + "&ws=" + String(WS) + "&t=" + String(T);
        stroka += "&np=" + String(NP) + "&fw=" + String(FW) + "&md=" + String(MD) + "&dstatus=" + String(Dv);

        Serial.println("W14=" + stroka);

        memset(msg, '\0', 1100);
        numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

        stroka = "";
        z = strlen(msg);
        for (int i = z - 30; i < numdata; i++) stroka += msg[i];

        Serial.println("\nW15: " + String(numdata));
        Serial.println("\nW16:");
        Serial.println("<" + stroka + ">");
        Serial.println(" ");

        if (numdata != 0)                   // Есть ответ
        {
          // Разбор ответного сообщения и присвоение значения
          int j;
          int m;
          int q;
          j = stroka.indexOf("W:");
          m = stroka.indexOf("T:");
          q = stroka.indexOf("_(]AdIXW");
          if (q > 0)
          {
            if (j >= 0)
            {
              cena1Litra = (stroka.substring(j + 2, m - 1)).toInt();
              if (cena1Litra != cenaLitr)
              {
                cenaLitr = cena1Litra;
                File fCena1L = SD.open("Cena1L.txt", FILE_WRITE);
                fCena1L.seek(0);
                fCena1L.println(cenaLitr);         // Запись 3 руб. за 1 литр
                fCena1L.flush();
                fCena1L.close();                   // Закрытие файла
              }
              Serial.println("cL=" + String(cenaLitr));
            }
            if (m > 0)
            {
              tempKrit = (stroka.substring(m + 2, q)).toInt();
              Serial.println("tempKr=" + String(tempKrit));
            }
          }
        }
        beginOtpravki = millis();
        Serial.println(String(millis()));

        //    digitalWrite(blokbanknotePin, LOW);  // Разблокировка приема купюр
        //    digitalWrite(blokcoinPin, LOW);      // Разблокировка приема монет
        //        beginOtpravki = millis();              // Начало отсчета времени до отправки на сервер
        //        Serial.println(String(millis()));


        if (kluchAdmin)                        // Главный ключ ?

        { //@@@@@@@@@@@@@@@ ДА-главный ключ @@@@@@@@@@@@@@@@@@@@@@@@

          Serial.print("W10=");
          for (int i = 0; i < 8; i++)
          {
            Serial.print(addr[i], HEX);
            Serial.print(" ");
          }
          Serial.println(" ");
          /*
                    // Блокирование приема денег и использования TM клиентов
                    detachInterrupt(2);                  // Выключение обработки прерывания
                    detachInterrupt(3);                  // Выключение обработки прерывания
                    digitalWrite(relay2Pin, LOW);        // Блокировка приема монет/купюр
          */
          digitalWrite(zamokPin, LOW);         // Открывание замка аппарата
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  Замок открыт! ");
          lcd.setCursor(0, 1);
          lcd.print("                ");
          Serial.println("Adm W12");

          while (digitalRead(dverPin)) ;       // Зацикливание, пока дверца закрыта
          //          while (!digitalRead(dverPin)) ;     // Зацикливание, пока дверца закрыта
          if (!digitalRead(dverPin))           // Дверца открыта
            //          if (digitalRead(dverPin))         // Дверца открыта
          {
            digitalWrite(zamokPin, HIGH);      // Закрывание замка аппарата
            Serial.print("zamokPin=");
            Serial.println(digitalRead(zamokPin));    // Должно быть: zamokPin=1
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("  Не  забудьте  ");
            lcd.setCursor(0, 1);
            lcd.print("закрыть дверцу !");
            Serial.println("Adm W13");
            while (!digitalRead(dverPin)) ;    // Зацикливание, пока открыта дверца
            //            while (digitalRead(dverPin)) ;  // Зацикливание, пока открыта дверца

            resetFunc();                       // Перезагрузка
            return;
          }
        } //@@@@@@@@@@@@@@@ Конец-Главный ключ @@@@@@@@@@@@@@@@@@@@@
        else
        { //@@@@@@@@@@@@@@@ Нет - НЕ Главный ключ @@@@@@@@@@@@@@@@@@

          priznClient = false;                 // true - наличие файла в папке Client
          priznInkass = false;                 // true - наличие файла в папке Inkass
          kluchPolzov = "";
          Serial.print("TM=");
          for (int i = 1; i < 8; i++)
          {
            kluchPolzov += addr[i];
            Serial.print(addr[i], HEX);
            Serial.print(" ");
          }
          Serial.println("");

          kkk = false;            // Прислонен ключ

          nameInkass = poiskInkass(kluchPolzov);
          if (priznInkass)                                 // Найден файл ID.txt в папке Inkass
          {
            /*
                      // Чтение данных из найденного файла
                      File fInkass = SD.open(pathInkass+nameInkass);   // Открытие файла для чтения
                      fInkass.seek(17);
                      bb = "";
                      while (fInkass.peek() != 0x0D)
                        {
                        abc = fInkass.read();
                         bb += abc;
                       }
                       fInkass.read();                                // Пропуск байта 0x0D
                       fInkass.read();                                // Пропуск байта 0x0A

                       hh = "";
                       while (fInkass.peek() != 0x0D)
                       {
                         abc = fInkass.read();
                         hh += abc;
                       }
                       NNNN = hh.toInt();                             // Номер посылки
                       fInkass.read();                                // Пропуск байта 0x0D
                       fInkass.read();                                // Пропуск байта 0x0A

                       hh = "";
                       while (fInkass.peek() != 0x0D)
                       {
                         abc = fInkass.read();
                         hh += abc;
                       }
                       upd = hh.toInt();                              // Флаг посылки
                       fInkass.close();
            */
            //            priznInkass = true;                          // Есть файл в папке Inkass
            kluch = 3;
          }

          if (kluch != 3)
          {
            nameClient = poiskClient(kluchPolzov);
            if (priznClient)                               // Найден файл ID.txt в папке Client
            {
              // Чтение данных из найденного файла
              File fClient = SD.open(pathClient + nameClient); // Открытие файла для чтения
              fClient.seek(17);
              bb = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                bb += abc;
              }
              balans = bb.toInt();                         // Баланс клиента

              /*
                            fClient.read();                              // Пропуск байта 0x0D
                            fClient.read();                              // Пропуск байта 0x0A

                            hh = "";
                            while (fClient.peek() != 0x0D)
                            {
                              hh += fClient.read();
                            }
                            literClient = hh.toInt();
                            fClient.read();                              // Пропуск байта 0x0D
                            fClient.read();                              // Пропуск байта 0x0A

                            hh = "";
                            while (fClient.peek() != 0x0D)
                            {
                              hh += fClient.read();
                            }
                            avansClient = hh.toInt();
                            fClient.read();                              // Пропуск байта 0x0D
                            fClient.read();                              // Пропуск байта 0x0A

                            hh = "";
                            while (fClient.peek() != 0x0D)
                            {
                              hh += fClient.read();
                            }
                            NNNN = hh.toInt();                           // Номер посылки
                            fClient.read();                              // Пропуск байта 0x0D
                            fClient.read();                              // Пропуск байта 0x0A

                            hh = "";
                            while (fClient.peek() != 0x0D)
                            {
                              hh += fClient.read();
                            }
                            upd = hh.toInt();                            // Флаг посылки
              */
              fClient.close();
              //              priznClient = true;                        // Есть файл в папке Client
              kluch = 2;
            }
          }
          if ((kluch != 2) && (kluch != 3))
          {
            // Отправка addr на сервер и получение ответа
            //            lcd.clear();           // Очистка экрана
            //            lcd.setCursor(0, 0);
            //            lcd.print("  Подождите ... ");
            //            lcd.setCursor(0, 1);
            //            lcd.print("                ");

            for (int v = 1; v < 4; v++) // 3 попытки !!!!!!!!!!!!
            {
              Serial.println("P=" + String(v));
              stroka = stroka0 + "U=" + numApp + "&T=";
              stroka += (kluchPolzov + P1);

              Serial.println("W14=" + stroka);
              memset(msg, '\0', 1100);
              numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);
              delay(3000);
              if (numdata == 0)
              {
                kluch = 4;  // Нет связи с сервером
              }
              else
              {
                // Разбор ответного сообщения и присвоение значения
                int j;
                int m;
                int p;
                int q;
                String sss = "";

                stroka = "";
                z = strlen(msg);
                for (int i = z - 30; i < numdata; i++) stroka += msg[i];

                Serial.println("\nN: " + String(numdata));
                Serial.println("\nW16:");
                Serial.println("<" + stroka + ">");
                Serial.println("$$$$$$$$$$$$$$$$$$$$$$");

                j = stroka.indexOf("ID:");
                m = stroka.indexOf("L:");
                p = stroka.indexOf("B:");
                q = stroka.indexOf("_(]AdIXW");
                if (q < 0)
                {
                  kluch = 4;  // Неверный ответ
                }
                else
                {
                  ID = stroka.substring(j + 3, m - 1);  // Без запятой в конце
                  sss = stroka.substring(m + 2, p - 1); // Без запятой в конце
                  kluch = sss.toInt();
                  sss = stroka.substring(p + 2, q);
                  balans = sss.toInt();
                  Serial.println("ID=" + ID + " kluch=" + String(kluch) + " balans=" + String(balans));
                  break;                                    // Выход, если Ok!
                }
              }
            }
          }

          // 0 - таблетка не действует
          // 1 - ключ администратора
          // 2 - ключ покупателя (balans)
          // 3 - ключ инкассатора
          // 4 - нет связи с сервером

          ds.reset();

          switch (kluch)
          {
            case 0:                                        // Нет в базе данных/Заблокирован
              {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Ключ не считан! ");
                lcd.setCursor(0, 1);
                lcd.print(" Повторите ...  ");
                Serial.println("W18");
                delay(4000);
                ds.reset_search();
                kkk = true;                                  // Наличные
                variant = 0;
              }
              break;
            case 1:                                        // Ключ администратора
              {
                Serial.print("W10=");
                for (int i = 0; i < 8; i++)
                {
                  EEPROM.write(i, addr[i]);
                  Serial.print(addr[i], HEX);
                  Serial.print(" ");
                }
                Serial.print("");

                digitalWrite(zamokPin, LOW);                 // Открывание замка аппарата
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("  Замок открыт! ");
                lcd.setCursor(0, 1);
                lcd.print("                ");
                Serial.println("Adm W12");

                while (digitalRead(dverPin)) ;     // Зацикливание, поки дверца закрыта
                //              while (!digitalRead(dverPin)) ;     // Зацикливание, поки дверца закрыта
                if (!digitalRead(dverPin))         // Дверца открыта
                  //              if (digitalRead(dverPin))         // Дверца открыта
                {
                  digitalWrite(zamokPin, HIGH);    // Закрывание замка аппарата
                  Serial.print("zamP=");
                  Serial.println(digitalRead(zamokPin));     // Должно быть: zamokPin=0
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("  Не  забудьте  ");
                  lcd.setCursor(0, 1);
                  lcd.print("закрыть дверцу !");
                  Serial.println("W13");
                  while (!digitalRead(dverPin)) ;  // Зацикливание, пока не закрыта дверца
                  //                while (digitalRead(dverPin)) ; // Зацикливание, пока не закрыта дверца

                  resetFunc();                // Перезагрузка
                  return;
                }
              }
              break;
            case 2:                   // Ключ покупателя воды
              {
                if (!digitalRead(nowaterPin)) // Нет воды
                {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("    Извините!   ");
                  lcd.setCursor(0, 1);
                  lcd.print("   Нет воды ... ");
                  Serial.println("Net vody");
                  //                detachInterrupt(2);         // Выключение обработки прерывания
                  //                detachInterrupt(3);         // Выключение обработки прерывания
                  ds.reset_search();
                  delay(3000);
                  kkk = true;
                  variant = 0;

                  return;
                }

                if (!digitalRead(dverPin))    // Дверца открыта
                {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Открыта дверца! ");
                  lcd.setCursor(0, 1);
                  lcd.print("Налив невозможен");
                  Serial.println("Otkr dver");
                  //                detachInterrupt(2);         // Выключение обработки прерывания
                  //                detachInterrupt(3);         // Выключение обработки прерывания
                  ds.reset_search();
                  delay(3000);
                  kkk = true;
                  variant = 0;

                  return;
                }

                // Использование balans
                variant = 1;
                literClient = 0;
                kkk = false;                           // Прислонен ключ
                //              digitalWrite(blokbanknotePin, LOW);    // Разблокировка приема купюр
                //              digitalWrite(blokcoinPin, LOW);        // Разблокировка приема монет
                ds.reset_search();

                Serial.println("var=" + String(variant) + "   bal=" + String(balans));
                delay(200);

                time_start = millis();                     // Время начала отсчета тайм-аута

                //              return;                                  // На ветку
              }
              break;
            case 3:                                        // Ключ инкассатора
              {
                Serial.println("Key Inkas");

                for (byte i = 0; i < 8; i++)                 // Запоминание ключа инкассатора
                {
                  inkass[i] = addr[i];
                }
                co = "";
                fCoin.seek(0);
                while (fCoin.peek() != 0x0D)
                {
                  abc = fCoin.read();
                  co += abc;
                }
                coinItog = co.toInt();
                ba = "";
                fBanknote.seek(0);
                while (fBanknote.peek() != 0x0D)
                {
                  abc = fBanknote.read();
                  ba += abc;
                }
                banknoteItog = ba.toInt();
                itogo = coinItog + banknoteItog;

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Монеты:" + co + " р.");
                lcd.setCursor(0, 1);
                lcd.print("Купюры:" + ba + " р.");
                Serial.println("W07=" + co + "W08=" + ba);
                ds.reset_search();
                delay(10000);
                variant = 3;                                 // Инкассация
                time_inkass = millis();                      // Начало отсчета времени
              }
              break;
            case 4:                                        // Нет связи с сервером
              {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("   Нет  связи   ");
                lcd.setCursor(0, 1);
                lcd.print("   с сервером   ");
                Serial.println("W19");
                ds.reset_search();
                delay(3000);
              }
              break;
          }
          //          digitalWrite(zamokPin, HIGH);  // Закрывание замка аппарата
        } //@@@@@@@@@@@@@@@ Конец - НЕ Главный ключ @@@@@@@@@@@@@@@@
      }          // Конец - проверка CRC и проверка кода устройства * * * * * * * * * *
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  Контр. сумма  ");
        lcd.setCursor(0, 1);
        lcd.print(" ключа неверна! ");
        Serial.println("W20");
        ds.reset_search();
        ds.reset();
        delay(3000);
        kluch = 0;
      }
    }            // Конец - Устройство найдено
    else
    {
      ds.reset_search();          // Устройство НЕ найдено
      ds.reset();
    }
  }              // Конец - if(variant==0)

  /*
    //+++++++++++++++++++++ НЕТ ВОДЫ +++++++++++++++++++++++++++++++++++++
    if(digitalRead(nowaterPin)==HIGH)        // Нет воды
    {
    //    digitalWrite(relay4Pin, HIGH);       // Включение розетки 220 V
      no_water = true;
      // Блокирование приема денег и использования TM клиентов
    //    digitalWrite(blokbanknotePin, HIGH); // Блокировка приема купюр
    //    digitalWrite(blokcoinPin, HIGH);     // Блокировка приема монет
      // Сообщение об отсутствии воды на сервер
      lcd.clear();           // Очистка экрана
      lcd.setCursor(0, 0);
      lcd.print("     Н Е Т      ");
      lcd.setCursor(0, 1);
      lcd.print("    В О Д Ы !   ");
      delay(1000);
      return;
    }
  */

  switch (variant)
  {
    // ------- Режим ожидания внесения денег или прислонения таблетки -----------
    case 0:
      {
        literClient = 0;

        if (!digitalRead(nowaterPin)) // Нет воды
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("    Извините!   ");
          lcd.setCursor(0, 1);
          lcd.print("   Нет воды ... ");
          Serial.println("Net vody");
          //        detachInterrupt(2);         // Выключение обработки прерывания
          //        detachInterrupt(3);         // Выключение обработки прерывания
          delay(3000);
          kkk = true;
          variant = 0;

          return;
        }

        if (!digitalRead(dverPin))    // Дверца открыта
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Открыта дверца! ");
          lcd.setCursor(0, 1);
          lcd.print("Налив невозможен");
          Serial.println("Otkr dver");
          //        detachInterrupt(2);         // Выключение обработки прерывания
          //        detachInterrupt(3);         // Выключение обработки прерывания
          delay(3000);
          kkk = true;
          variant = 0;

          return;
        }

        // Есть вода и дверца закрыта
        attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
        attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW
        delay(20);
        //      digitalWrite(blokbanknotePin, LOW); // Разблокировка приема купюр
        //      digitalWrite(blokcoinPin, LOW);     // Разблокировка приема монет

        if ((pulse_coin + pulse_banknote) > 0)
        {
          variant = 1;
          literClient = 0;
          time_start = millis();              // Время начала отсчета тайм-аута
          return;
        }
        else
        {
          if (kkk1)
          {
            lcd.clear();           // Очистка экрана
            lcd.setCursor(0, 0);
            lcd.print("  Внесите аванс ");
            lcd.setCursor(0, 1);
            lcd.print(" 1 литр=" + String(cenaLitr) + " руб.");
          }
          else
          {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("                ");
            lcd.setCursor(0, 1);
            lcd.print("Прислоните  ключ");
          }
          delay(2000);
          kkk1 = !kkk1;
        }
      }
      break;

    // ------- Внесена монета или купюра ----------------------------------------
    case 1:
      {
        while (true)
        {
          //        attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
          //        attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW

          if (!kkk && ((millis() - time_start) > timeSec15)) // 15 с. до прекращения налива
          {
            //          detachInterrupt(2);         // Выключение обработки прерывания
            //          detachInterrupt(3);         // Выключение обработки прерывания
            //          lcd.clear();
            //          lcd.setCursor(0, 0);
            //          lcd.print("  Подождите ... ");
            //          lcd.setCursor(0, 1);
            //          lcd.print("                ");
            //          Serial.println("W21");

            if (priznClient)               // Есть файл в папке Client
            {
              long avansCl;
              long literCl;
              // Чтение данных прошлого сохранения
              File fClient = SD.open(pathClient + nameClient, FILE_WRITE); // Открытие для записи
              fClient.seek(17);
              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              balans = hh.toInt();                  // Баланс клиента с TM
              fClient.read();                       // Пропуск байта 0x0D
              fClient.read();                       // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              literCl = hh.toInt();            // Добавление литров к предыдущим литрам
              fClient.read();                       // Пропуск байта 0x0D
              fClient.read();                       // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              avansCl = hh.toInt();                 // Предыдущие пополнения
              fClient.read();                       // Пропуск байта 0x0D
              fClient.read();                       // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              fClient.read();                       // Пропуск байта 0x0D
              fClient.read();                       // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              NNNN = hh.toInt();                    // Номер посылки
              // Добавление новых данных с upd=1
              fClient.seek(17);
              fClient.println(balans + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
              fClient.println(literCl + literClient);                 // Новые литры
              literClient = 0;
              fClient.println(avansCl + pulse_coin + 10 * pulse_banknote); // Новое пополнение
              itogo = SummaKop();                   // + сохранение в SD, очистка счетчиков
              fClient.println(itogo);               // Копилка
              fClient.println(NNNN);                // Существующий номер посылки
              fClient.println(1);                   // upd = 1
              fClient.flush();
              fClient.close();
            }
            else                                    // Нет файла в папке Client
            {
              NNNN = NNNNrabota();                  // Новый номер посылки
              // Создание файла Client/ID.TXT и запись в него данных о действиях клиента
              File fClient = SD.open(pathClient + ID + ".txt", FILE_WRITE); // Создание для записи
              fClient.seek(0);
              for (int i = 0; i < 8; i++)
              {
                fClient.write(addr[i]);
                fClient.write(" ");
              }
              fClient.write(" ");
              fClient.println(balans + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
              fClient.println(literClient);       // Новые литры клиента
              literClient = 0;
              fClient.println(pulse_coin + 10 * pulse_banknote);   // Новое пополнение
              itogo = SummaKop();                 // + сохранение в SD, очистка счетчиков
              fClient.println(itogo);             // Копилка
              fClient.println(NNNN);              // Существующий номер посылки
              fClient.println(0);                 // upd = 0
              fClient.flush();
              fClient.close();

              Serial.println("   FileClient: " + pathClient + ID + ".txt");
            }

            variant = 0;
            kkk = true;        // Наличные
            attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
            attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW

            return;
          }

          if ((millis() - time_start) > timeSbrosa) // Превышено время бездействия?
          {
            //          detachInterrupt(2);         // Выключение обработки прерывания
            //          detachInterrupt(3);         // Выключение обработки прерывания
            //          lcd.clear();
            //          lcd.setCursor(0, 0);
            //          lcd.print("  Подождите ... ");
            //          lcd.setCursor(0, 1);
            //          lcd.print("                ");
            //          Serial.println("W21");

            // Клиент без TM (наличка)
            if (SD.exists("Nalich.txt"))           // Существует файл Nalich.txt
            {
              long avansCl;
              // Считывание данных файла
              File fNalich = SD.open("Nalich.txt", FILE_WRITE);
              fNalich.seek(0);
              hh = "";
              while (fNalich.peek() != 0x0D)       // Пропуск байта 0x0D
              {
                abc = fNalich.read();
                hh += abc;
              }
              avansCl = hh.toInt();           // + Авансы предыдущих клиентов
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)       // Пропуск байта 0x0D
              {
                abc = fNalich.read();
                hh += abc;
              }
              literClient += hh.toInt();           // + Литры предыдущих клиентов
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)       // Пропуск байта 0x0D
              {
                abc = fNalich.read();
                hh += abc;
              }
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)
              {
                abc = fNalich.read();
                hh += abc;
              }
              NNNN = hh.toInt();                   // Номер посылки

              fNalich.seek(0);
              fNalich.println(avansCl + pulse_coin + 10 * pulse_banknote);
              fNalich.println(literClient);
              literClient = 0;
              itogo = SummaKop();                  // + сохранение в SD, очистка счетчиков
              fNalich.println(itogo);
              fNalich.println(NNNN);
              fNalich.println(1);
              fNalich.flush();
              fNalich.close();

              Serial.println("W33");
            }
            else                                   // Создание Nalich.txt
            {
              NNNN = NNNNrabota();                 // Новый номер посылки
              File fNalich = SD.open("Nalich.txt", FILE_WRITE);
              fNalich.seek(0);
              fNalich.println(pulse_coin + 10 * pulse_banknote);
              fNalich.println(literClient);
              literClient = 0;
              itogo = SummaKop();                  // + сохранение в SD, очистка счетчиков
              fNalich.println(itogo);
              fNalich.println(NNNN);
              fNalich.println(0);
              fNalich.flush();
              fNalich.close();

              Serial.println("W34");
            }
            variant = 0; // default
            kkk = true;  // Аванс
            attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
            attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW

            return;
          }

          if (kkk && ((pulse_coin + 10 * pulse_banknote - literClient * cenaLitr) > 0)) // Есть наличные
          {
            kluch = 5;

            if (ds.search(addr))                      // Найдено устройство?
            { // Устройство найдено
              Serial.println("The device TM was found");
              lcd.clear();           // Очистка экрана
              lcd.setCursor(0, 0);
              lcd.print("  Подождите ... ");
              lcd.setCursor(0, 1);
              lcd.print("                ");
              // проверка CRC и кода устройства
              if ((OneWire::crc8(addr, 7) == addr[7]) && (addr[0] == 0x01))
              {
                //              detachInterrupt(2);         // Выключение обработки прерывания
                //              detachInterrupt(3);         // Выключение обработки прерывания
                kluchPolzov = "";
                Serial.print("TM=");
                for (int i = 1; i < 8; i++)
                {
                  kluchPolzov += addr[i];
                  Serial.print(addr[i], HEX);
                  Serial.print(" ");
                }
                Serial.println("");

                nameClient = poiskClient(kluchPolzov);
                if (priznClient)                               // Найден файл ID.txt в папке Client
                {
                  long balansCl;
                  long literCl;
                  long avansCl;
                  // Чтение данных из найденного файла
                  File fClient = SD.open(pathClient + nameClient, FILE_WRITE); // Открытие для записи
                  fClient.seek(17);
                  bb = "";
                  while (fClient.peek() != 0x0D)
                  {
                    abc = fClient.read();
                    bb += abc;
                  }
                  balansCl = bb.toInt() + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr;
                  fClient.read();                              // Пропуск байта 0x0D
                  fClient.read();                              // Пропуск байта 0x0A

                  hh = "";
                  while (fClient.peek() != 0x0D)
                  {
                    abc = fClient.read();
                    hh += abc;
                  }
                  literCl = hh.toInt();                        // Литры предыдущего налива
                  fClient.read();                              // Пропуск байта 0x0D
                  fClient.read();                              // Пропуск байта 0x0A

                  hh = "";
                  while (fClient.peek() != 0x0D)
                  {
                    abc = fClient.read();
                    hh += abc;
                  }
                  avansCl = hh.toInt();
                  fClient.read();                              // Пропуск байта 0x0D
                  fClient.read();                              // Пропуск байта 0x0A

                  hh = "";
                  while (fClient.peek() != 0x0D)
                  {
                    abc = fClient.read();
                    hh += abc;
                  }
                  fClient.read();                              // Пропуск байта 0x0D
                  fClient.read();                              // Пропуск байта 0x0A

                  hh = "";
                  while (fClient.peek() != 0x0D)
                  {
                    abc = fClient.read();
                    hh += abc;
                  }
                  NNNN = hh.toInt();                           // Номер посылки

                  // Добавление данных в существующий файл
                  fClient.seek(17);
                  fClient.println(balansCl);
                  fClient.println(literCl);           // Старые литры клиента
                  fClient.println(avansCl + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
                  itogo = SummaKop();                 // + сохранение в SD, очистка счетчиков
                  fClient.println(itogo);             // Копилка
                  fClient.println(NNNN);              // Существующий номер посылки
                  fClient.println(1);                 // upd = 1
                  fClient.flush();
                  fClient.close();

                  Nalichka();

                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Баланс пополнен!");
                  lcd.setCursor(0, 1);
                  lcd.print("Баланс: " + String(balansCl) + " р.");
                  Serial.println("23");
                  kkk = true;                // Наличные
                  variant = 0;

                  return;
                }
                else                                  // Нет файла ID.txt в папке Client
                {
                  // Запрос на сервер о ключе
                  for (int v = 1; v < 4; v++) // 3 попытки !!!!!!!!!!!!
                  {
                    Serial.println("Pop " + String(v));
                    stroka = stroka0 + "U=" + numApp + "&T=";
                    stroka += (kluchPolzov + P1);
                    Serial.println("str=" + stroka);

                    memset(msg, '\0', 1100);
                    numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);
                    Serial.println("\W15: " + String(numdata));
                    if (numdata == 0) continue;
                    stroka = "";
                    z = strlen(msg);
                    for (int i = z - 30; i < numdata; i++) stroka += msg[i];

                    Serial.println("\nW16:");
                    Serial.println("<" + stroka + ">");
                    // Разбор ответного сообщения и присвоение значения
                    int j;
                    int m;
                    int p;
                    int q;
                    String sss = "";
                    long balansCl;
                    j = stroka.indexOf("ID:");
                    m = stroka.indexOf("L:");
                    p = stroka.indexOf("B:");
                    q = stroka.indexOf("_(]AdIXW");
                    if (q < 0) continue;                         // Неверный ответ

                    ID = stroka.substring(j + 3, m - 1);         // Без запятой в конце
                    sss = stroka.substring(m + 2, p - 1);        // Без запятой в конце
                    kluch = sss.toInt();
                    sss = stroka.substring(p + 2, q);
                    balansCl = sss.toInt();
                    Serial.println("ID=" + ID + " kluch=" + String(kluch) + " balans=" + String(balans));

                    // Если ключ клиента, то пополнение баланса значением avans
                    if (kluch == 2 || kluch == 0)  // Получен баланс ключа
                    {
                      NNNN = NNNNrabota();
                      // Создание и запись данных в файл Client/ID.txt
                      File fClient = SD.open(pathClient + ID + ".txt", FILE_WRITE); // Создание файла
                      fClient.seek(0);
                      for (int i = 0; i < 8; i++)
                      {
                        fClient.write(addr[i]);
                        fClient.write(" ");
                      }
                      fClient.write(" ");
                      balansCl = balansCl + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr;
                      fClient.println(balansCl);
                      fClient.println(0);
                      fClient.println(pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
                      itogo = SummaKop();                 // + сохранение в SD, очистка счетчиков
                      fClient.println(itogo);
                      fClient.println(NNNN);              // Номер посылки
                      fClient.println(0);                 // upd = 0
                      fClient.flush();
                      fClient.close();

                      Serial.println(" File: " + pathClient + ID + ".txt");

                      Nalichka();

                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("Баланс пополнен!");
                      lcd.setCursor(0, 1);
                      lcd.print("Баланс: " + String(balansCl) + " р.");
                      Serial.println("23");
                      kkk = true;                // Наличные
                      variant = 0;

                      return;
                    }
                    delay(2000);
                  }
                  lcd.clear();           // Очистка экрана
                  lcd.setCursor(0, 0);
                  lcd.print("Ключ не считан! ");
                  lcd.setCursor(0, 1);
                  lcd.print(" Повторите ...  ");
                  ds.reset();
                  ds.reset_search();
                  delay(3000);

                  attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
                  attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW

                  //                return;
                }
              }
              ds.reset();
            }
            else ds.reset_search();                 // Устройство НЕ найдено
          }

          // Подсчет допустимого расхода в литрах и квоты налива
          kolich = pulse_coin + 10 * pulse_banknote - literClient * cenaLitr; // Рублей
          if (kkk)                                  // Наличка
          {
            kolich = kolich / cenaLitr;             // Литров
          }
          else                                      // Ключ TM
          {
            kolich = (balans + kolich) / cenaLitr;  // Литров
          }
          kvota = min(kolich, 5);                   // 0 <= kvota <= 5 литров

          monitor02();         // Аванс=литры

          delay(50);

          if (kvota > 0)                            // Возможен налив воды?
          {
            if (digitalRead(puskPin) != clickPusk)  // Изменение положения кнопки
            {
              clickPusk = 1 - clickPusk;
              beginPusk = millis();
            }
            if (clickPusk == 1 && (millis() - beginPusk) > 100) // нажата более 0.1 сек
            {
              //            pulse_banknote = -1;
              //            pulse_coin = -1;
              //            detachInterrupt(2);         // Выключение обработки прерывания
              //            detachInterrupt(3);         // Выключение обработки прерывания
              //            digitalWrite(blokbanknotePin, HIGH);   // Блокировка купюр
              //            digitalWrite(blokcoinPin, HIGH);       // Блокировка монет
              clickPusk = 0;                           // Кнопка отжата
              pulse_flow = 0;                          // Очистка счетчика импульсов датчика воды
              attachInterrupt(0, count_flow, RISING);  // LOW  -> HIGH
              //            attachInterrupt(1, count_flow, RISING);  // LOW  -> HIGH
              pulseKvota = kvota / litrPulse;          // Число импульсов для налива воды
              Serial.println("W24=" + String(pulseKvota));

              delay(50);
              variant = 2;
              return;                                  // На ветку налива воды
            }
          }
        }
      }
      break;

    // --------------- Налив воды --------------------------------------------------
    case 2:
      {
        int j = 1;                           // Счетчик количества налитых литров
        int jImp;
        jImp = pulseLiter;                   // Число импульсов в 1 литре
        float nalivPulse = 0;                // Налито воды (float)
        float ostatok = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Идет налив воды ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        Serial.print("W25 ");

        digitalWrite(relay1Pin, HIGH);         // Реле насоса замкнуто

        while (pulse_flow < pulseKvota)        // Пока не отсчитает импульсы
        {
          nalivPulse = pulse_flow * litrPulse; // Налив воды
          if (!digitalRead(nowaterPin)) break; // Нет воды

          if (pulse_flow > jImp)               // Налито j литров
          {
            lcd.setCursor(7, 1);
            lcd.print(String(j) + " л.");
            Serial.println(String(j) + " L.");
            j++;
            jImp = j * pulseLiter;           // Число импульсов в j литрах
          }

          // Вариант с исключением дребезга
          if (pulse_flow >= pulseLiter / 2)    // Разрешение останова после налива 1 литра
          {
            if (digitalRead(puskPin) != clickPusk)  // Изменение положения кнопки
            {
              clickPusk = 1 - clickPusk;
              beginPusk = millis();
            }
            if (clickPusk == 1 && (millis() - beginPusk) > 100) // нажата более 0.1 сек
            {
              clickPusk == 0;
              break;
            }
          }
          /*
                    // Вариант с простым нажатием кнопки
                    if (digitalRead(puskPin) != clickPusk)        // Кнопка ПУСК нажата
                    {
                      clickPusk == 0;
                      kvota_real = j-1;                // Количество налитой воды (л.)
                      lcd.clear();           // Очистка экрана
                      break;
                    }
          */
        }

        digitalWrite(relay1Pin, LOW);         // Размыкание реле насоса
        delay(500);

        detachInterrupt(0);                    // Выключение обработки прерывания
        //      detachInterrupt(1);                    // Выключение обработки прерывания

        kvota = nalivPulse;
        ostatok = nalivPulse - kvota;
        if (ostatok > 0.2)
        {
          kvota++;
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  Налито: " + String(kvota) + " л.  ");
        Serial.println("Nalito: " + String(kvota) + " L.");

        delay(2000);

        pulse_flow = 0;                        // Очистка счетчика импульсов датчика воды

        literClient += kvota;                  // Количество литров налитой воды
        // Подсчет допустимого расхода в литрах
        kolich = pulse_coin + 10 * pulse_banknote - literClient * cenaLitr; // Рублей
        if (kkk)                                  // Наличка
        {
          kolich = kolich / cenaLitr;             // Литров
        }
        else                                      // Ключ TM
        {
          kolich = (balans + kolich) / cenaLitr;  // Литров
        }
        //      Serial.println("bal="+String(balans)+" av="+String(avans)+" kol="+String(kolich));

        if ((kolich == 0) || !digitalRead(nowaterPin)) // НЕТ литров или НЕТ воды
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  Подождите ... ");
          lcd.setCursor(0, 1);
          lcd.print("Идет сохранение ");
          Serial.println("W21");

          if (kkk && ((pulse_coin + 10 * pulse_banknote) > 0)) // Наличка
          {
            if (SD.exists("Nalich.txt"))           // Существует файл Nalich.txt
            {
              long avansCl;
              long literCl;
              // Чтение данных файла
              File fNalich = SD.open("Nalich.txt", FILE_WRITE);
              fNalich.seek(0);
              hh  = "";
              while (fNalich.peek() != 0x0D)
              {
                abc = fNalich.read();
                hh += abc;
              }
              avansCl = hh.toInt();     //  + avansClient;
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)
              {
                abc = fNalich.read();
                hh += abc;
              }
              literCl = hh.toInt();     //  + literClient;
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)
              {
                abc = fNalich.read();
                hh += abc;
              }
              fNalich.read();                      // Пропуск байта 0x0D
              fNalich.read();                      // Пропуск байта 0x0A

              hh = "";
              while (fNalich.peek() != 0x0D)
              {
                abc = fNalich.read();
                hh += abc;
              }
              NNNN = hh.toInt();                   // Номер посылки

              // Сохранение данных в файл Nalich.txt
              fNalich.seek(0);
              fNalich.println(avansCl + pulse_coin + 10 * pulse_banknote);
              fNalich.println(literCl + literClient);
              itogo = SummaKop();                  // + сохранение в SD, очистка счетчиков
              fNalich.println(itogo);
              fNalich.println(NNNN);
              fNalich.println(1);
              fNalich.flush();
              fNalich.close();

              Serial.println("W35");
            }
            else                                   // Нет файла Nalich.txt
            {
              // Сохранение на SD
              NNNN = NNNNrabota();                 // Получение уникального номера запроса
              File fNalich = SD.open("Nalich.txt", FILE_WRITE);
              fNalich.seek(0);
              fNalich.println(pulse_coin + 10 * pulse_banknote);  // Аванс
              fNalich.println(literClient);      // Литры
              itogo = SummaKop();                // + сохранение в SD, очистка счетчиков
              fNalich.println(itogo);
              fNalich.println(NNNN);             // Номер посылки
              fNalich.println(0);                // upd = 0
              fNalich.flush();
              fNalich.close();

              Serial.println("W37");
            }
          }

          if (!kkk && (((pulse_coin + 10 * pulse_banknote) > 0) || (literClient > 0))) // Клиент
          {
            if (priznClient)             // Существует файл ID.txt в папке Client
            {
              long balansCl;
              long literCl;
              long avansCl;
              // Чтение данных из найденного файла
              File fClient = SD.open(pathClient + nameClient, FILE_WRITE); // Открытие файла
              fClient.seek(17);
              bb = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                bb += abc;
              }
              balansCl = hh.toInt();                       // Баланс клиента
              fClient.read();                              // Пропуск байта 0x0D
              fClient.read();                              // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              literCl = hh.toInt();                        // Литры клиента
              fClient.read();                              // Пропуск байта 0x0D
              fClient.read();                              // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              avansCl = hh.toInt();                        // Пополнение клиента
              fClient.read();                              // Пропуск байта 0x0D
              fClient.read();                              // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              fClient.read();                              // Пропуск байта 0x0D
              fClient.read();                              // Пропуск байта 0x0A

              hh = "";
              while (fClient.peek() != 0x0D)
              {
                abc = fClient.read();
                hh += abc;
              }
              NNNN = hh.toInt();                           // Номер посылки

              // Добавление данных в существующий файл
              fClient.seek(17);
              fClient.println(balansCl + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
              fClient.println(literCl + literClient);                // Литры клиента
              fClient.println(avansCl + pulse_coin + 10 * pulse_banknote); // Пополнение клиента
              itogo = SummaKop();             // + сохранение в SD, очистка счетчиков
              fClient.println(itogo);         // Копилка
              fClient.println(NNNN);          // Существующий номер посылки
              fClient.println(1);             // upd = 1
              fClient.flush();
              fClient.close();
            }
            else                              // Нет файла ID.txt в папке Client
            {
              // Сохранение на SD
              NNNN = NNNNrabota();            // Получение уникального номера запроса
              // Запись данных в файл Client/ID.txt
              File fClient = SD.open(pathClient + ID + ".txt", FILE_WRITE); // Создание файла
              fClient.seek(0);
              for (int i = 0; i < 8; i++)
              {
                fClient.write(addr[i]);
                fClient.write(" ");
              }
              fClient.write(" ");
              fClient.println(balans + pulse_coin + 10 * pulse_banknote - literClient * cenaLitr);
              fClient.println(literClient);   // Новые литры клиента
              fClient.println(pulse_coin + 10 * pulse_banknote); // Пополнение клиента
              itogo = SummaKop();             // + сохранение в SD, очистка счетчиков
              fClient.println(itogo);         // Копилка
              fClient.println(NNNN);          // Номер посылки
              fClient.println(0);             // upd = 0
              fClient.flush();
              fClient.close();

              Serial.println("   FCl: " + pathClient + ID + ".txt");
            }
          }

          variant = 0;
          kkk = true;      // Наличные
          lcd.clear();
          return;
        }

        // Допустим расход воды
        variant = 1;
        time_start = millis();        // Время начала отсчета тайм-аута
        return;
      }
      break;

    // ===================== Инкассация =================================================
    case 3:
      {
        boolean kluchInkass = false;
        /*
              // Блокирование приема денег и использования TM клиентов
              detachInterrupt(2);                         // Выключение обработки прерывания
              detachInterrupt(3);                         // Выключение обработки прерывания
              digitalWrite(relay2Pin, LOW);               // Блокировка приема монет/купюр
        */
        while (millis() - time_inkass < timeInkass) // Пока не наступит время ожидания
        {
          //------------------ Проверка прислонения ключа инкассатора ---------------------

          if (ds.search(addr))                      // Найдено устройство?
          { // Устройство найдено
            Serial.println("The device TM was found");
            // проверка CRC
            if ((OneWire::crc8(addr, 7) == addr[7]) && (addr[0] == 0x01))
            { // Проверка кода устройства
              kluchInkass = true;
              for (int i = 0; i < 8; i++)           // Сравнение ключа с ключом инкассатора
              {
                if (addr[i] != inkass[i])
                {
                  kluchInkass = false;
                  break;
                }
              }
            }
            ds.reset();
          }
          else ds.reset_search();                   // Устройство НЕ найдено

          if (kluchInkass)                          // Повторно приложен ключ инкассатора!
          {
            digitalWrite(zamokPin, LOW);            // Открывание замка аппарата
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("  Замок открыт! ");
            lcd.setCursor(0, 1);
            lcd.print("Заберите  деньги");
            Serial.println("Inkas W12");
            //          delay(5000);

            while (digitalRead(dverPin)) ;          // Зацикливание, пока дверца закрыта
            //          while (!digitalRead(dverPin)) ;       // Зацикливание, пока дверца закрыта

            digitalWrite(zamokPin, HIGH);    // Закрывание замка аппарата
            //          digitalWrite(blokbanknotePin, HIGH);  // Блокировка приема купюр
            //          digitalWrite(blokcoinPin, HIGH);      // Блокировка приема монет

            // Обновление файлов Coin.txt Banknote.txt
            coinItog = 0;
            rabotaCoin();
            banknoteItog = 0;
            rabotaBanknote();

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("  Не  забудьте  ");
            lcd.setCursor(0, 1);
            lcd.print("закрыть дверцу !");
            Serial.println("W26");

            // Сохранение данных инкассации
            if (priznInkass)                        // Есть файл в папке Inkass
            {
              // Чтение данных прошлой инкассации
              File fInkass = SD.open(pathInkass + nameInkass, FILE_WRITE); // Открытие файла
              fInkass.seek(17);
              hh = "";
              while (fInkass.peek() != 0x0D)
              {
                abc = fInkass.read();
                hh += abc;
              }
              itogo += hh.toInt();                  // Добавление к предыдущей сумме инкассации
              fInkass.read();                       // Пропуск байта 0x0D
              fInkass.read();                       // Пропуск байта 0x0A

              hh = "";
              while (fInkass.peek() != 0x0D)
              {
                abc = fInkass.read();
                hh += abc;
              }
              NNNN = hh.toInt();                    // Номер посылки
              // Добавление новых данных с upd=1
              fInkass.seek(17);
              fInkass.println(itogo);               // Общая сумма инкассации
              fInkass.println(NNNN);                // Существующий номер посылки
              fInkass.println(1);                   // upd = 1
              fInkass.flush();
              fInkass.close();
            }
            else                                    // Отправка данных инкассации на сервер
            {
              NNNN = NNNNrabota();                  // Новый номер посылки
              upd = 0;
              // Запись данных в файл Inkass/ID.txt
              File fInkass = SD.open(pathInkass + ID + ".txt", FILE_WRITE); // Создание файла
              fInkass.seek(0);
              for (int i = 0; i < 8; i++)
              {
                fInkass.write(addr[i]);
                fInkass.write(" ");
              }
              fInkass.write(" ");
              fInkass.println(itogo);
              fInkass.println(NNNN);                // Существующий номер посылки
              fInkass.println(0);                   // upd = 0
              fInkass.flush();
              fInkass.close();

              Serial.println("   FInk: " + pathInkass + ID + ".txt");
            }
            delay(4000);

            while (!digitalRead(dverPin)) ;   // Зацикливание, пока не закрыта дверца
            //          while (digitalRead(dverPin));   // Зацикливание, пока не закрыта дверца

            resetFunc();                      // Перезагрузка
            return;
          }

          //-------------------- Проверка нажатия кнопки ПУСК -------------------------------
          if (digitalRead(puskPin) != clickPusk)  // Изменение положения кнопки
          {
            clickPusk = 1 - clickPusk;
            beginPusk = millis();
          }
          if (clickPusk == 1 && (millis() - beginPusk) > 100) // нажата более 0.1 сек
          {
            lcd.clear();           // Очистка экрана
            lcd.setCursor(0, 0);
            lcd.print("  Перезагрузка  ");
            lcd.setCursor(0, 1);
            lcd.print("   аппарата ... ");
            Serial.println("Ink W12");
            delay(3000);

            resetFunc();                         // Перезагрузка
            return;                              // На ветку налива воды
          }
        }
        /*
              // Разблокирование приема денег и использования TM
              digitalWrite(relay2Pin, HIGH);
              delay(5000);
              attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
              attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW
        */
        kkk = true;                              // Наличные
        variant = 0;
      }
      break;
  }

  // Проверка нижнего датчика воды
  if (!digitalRead(nowaterPin))                  // Нет воды
    //  {
    //    // Разблокирование приема денег и использования TM
    //    digitalWrite(relay2Pin, HIGH);
    //    delay(2000);
    //    attachInterrupt(2, count_banknote, FALLING); // HIGH -> LOW
    //    attachInterrupt(3, count_coin,     FALLING); // HIGH -> LOW
    //  }
    //  else                                           // Нет воды
  {
    /*
        // Блокирование приема денег и использования TM клиентов
        detachInterrupt(2);                          // Выключение обработки прерывания
        detachInterrupt(3);                          // Выключение обработки прерывания
        digitalWrite(relay2Pin, LOW);                // Блокировка приема монет/купюр
    */
    // Сообщение об отсутствии воды на сервер
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("     Н Е Т      ");
    lcd.setCursor(0, 1);
    lcd.print("    В О Д Ы !   ");
    Serial.println("no wody");
    delay(2000);
    variant = 0; // default
    kkk = true;  // Аванс
  }

  // Проверка критичной температуры

  /*
      dtemp.reset();
      dtemp.select(addrT);
      dtemp.write(0x44,1); // запускаем конвертацию
      delay(750); // ждем 750ms
      dtemp.reset();
      dtemp.select(addrT);
      dtemp.write(0xBE); // считываем ОЗУ датчика
      for (byte i = 0; i < 9; i++)
      { // обрабатываем 9 байт
        datad[i] = dtemp.read();
    //      Serial.print(datad[i], HEX);
    //      Serial.print(" ");
      }
      // высчитываем температуру :)
      LowByte = datad[0];
    //    Serial.print("LB=");
    //    Serial.print(LowByte,HEX);
      HighByte = datad[1];
    //    Serial.print("   HB=");
    //    Serial.print(HighByte,HEX);
      T = ((HighByte << 8)+LowByte)/2;        // Температура
  */

  T = 25;

  if (T < tempKrit)
  {
    Serial.print("Kr_temp=");
    Serial.print(T);
    digitalWrite(relay3Pin, LOW);          // Включение вентилятора
  }
  if (T > tempKrit + 5)
  {
    digitalWrite(relay3Pin, HIGH);         // Выключение вентилятора
  }

  if (digitalRead(fulltankPin))            // Полный бак
  {
    digitalWrite(relay4Pin, LOW);          // Отключение розетки 220 V
  }
  else
  {
    digitalWrite(relay4Pin, HIGH);         // Включение розетки 220 V
  }

  if (!digitalRead(littlewaterPin))        // Мало воды
  {
    digitalWrite(relay4Pin, HIGH);         // Включение розетки 220 V
  }
}

// Получение количества импульсов принятой купюры
void count_banknote()
{
  pulse_banknote++;           // Увеличение на 1 числа импульсов
}

// Получение количества импульсов принятой монеты
void count_coin()
{
  pulse_coin++;           // Увеличение на 1 числа импульсов
}

// Получение количества импульсов датчика воды
void count_flow()
{
  pulse_flow++;
}


// Информация об авансе и предложение о нажатии ПУСК для налива квоты
void monitor02()
{
  long gggg;
  gggg = pulse_coin + 10 * pulse_banknote - literClient * cenaLitr;
  lcd.clear();
  lcd.setCursor(0, 0);
  if (kkk)
  {
    lcd.print(String(gggg) + " руб.=" + String(kolich) + " л.");
  }
  else
  {
    lcd.print(String(balans + gggg) + " руб.=" + String(kolich) + " л.");
  }
  if (kolich > 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("ПУСК-налив " + String(kvota) + " л.");
  }
}


/*
  // Информация об авансе и предложение о нажатии ПУСК для налива квоты
  void monitor02()
  {
  if (kkk)                    // Наличка
  {
    if (literClient == 0)     // Не было налива
    {
      if (kkk2)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(String(avans) + " руб.=" + String(kolich) + " л.");
        lcd.setCursor(0, 1);
        lcd.print(" 1 литр=" + String(cenaLitr) + " руб.");
      }
      else
      {
        if (kolich > 0)         // Можно наливать воду
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Пополните аванс ");
          lcd.setCursor(0, 1);
          lcd.print("либо  ПУСК-налив");
        }
        else                    // 0 литров для налива
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Пополните аванс ");
          lcd.setCursor(0, 1);
          lcd.print(" 1 литр=" + String(cenaLitr) + " руб.");
        }
      }
      delay(2000);
      kkk2 = !kkk2;
    }
    else                      // Был налив
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String(avans) + " руб.=" + String(kolich) + " л.");
      if (kolich > 0)
      {
        lcd.setCursor(0, 1);
        lcd.print("ПУСК-налив " + String(kvota) + " л.");
      }
    }
  }
  else                        // Прислонен ключ
  {
    if (literClient == 0)     // Не было налива
    {
      if (kkk2)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(String(balans) + " руб.=" + String(kolich) + " л.");
        lcd.setCursor(0, 1);
        lcd.print(" 1 литр=" + String(cenaLitr) + " руб.");
      }
      else
      {
        if (kolich > 0)         // Можно наливать воду
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Пополните баланс");
          lcd.setCursor(0, 1);
          lcd.print("либо  ПУСК-налив");
        }
        else                    // 0 литров для налива
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Пополните баланс");
          lcd.setCursor(0, 1);
          lcd.print(" 1 литр=" + String(cenaLitr) + " руб.");
        }
      }
      kkk2 = !kkk2;
      delay(2000);
    }
    else                      // Был налив
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String(balans) + " руб.=" + String(kolich) + " л.");
      if (kolich > 0)
      {
        lcd.setCursor(0, 1);
        lcd.print("ПУСК-налив " + String(kvota) + " л.");
      }
    }
  }
  }
*/

// Информация о балансе и предложение о пополнении или сохранении
void monitor03()
{
  if (kkk2)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Баланс: " + String(balans) + " р.");
    lcd.setCursor(0, 1);
    lcd.print("Пополните баланс");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   либо ждите   ");
    lcd.setCursor(0, 1);
    lcd.print(" его сохранения ");
  }
  kkk2 = !kkk2;
  delay(2000);
}

//============== Функции для обмена информацией =====================

// Функция обновления файла Coin.txt
void rabotaCoin()
{
  fCoin.seek(0);
  fCoin.println(coinItog);
  fCoin.flush();
}

// Функция обновления файла Banknote.txt
void rabotaBanknote()
{
  fBanknote.seek(0);
  fBanknote.println(banknoteItog);
  fBanknote.flush();
}

// Функция отправки на сервер данных о балансе владельца TM
boolean otprClient()
{
  int z = 0;
  stroka = stroka0 + "U=" + numApp + "&T=" + kluchPolzov + "&B=" + bb + "&V=" + String(literClient);
  stroka += "&S=" + String(literClient * cenaLitr) + "&P=" + String(avansClient);
  stroka += "&m=" + String(itogo) + P2 + "&NN=" + String(NNNN) + "&upd=" + String(upd);

  Serial.println("W14=" + stroka);

  memset(msg, '\0', 1100);
  numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

  Serial.println("\nW15: " + String(numdata));
  Serial.println("\nW16:");
  Serial.println("<" + stroka + ">");

  if (numdata == 0)     // Ответ пустой
  {
    // Неудача в обновлении данных на сервере
    Serial.println("W27=" + String(millis()));
    return false;
  }

  // Анализ ответа от сервера
  stroka = "";
  z = strlen(msg);
  for (int i = z - 20; i < numdata; i++) stroka += msg[i];

  // Разбор ответного сообщения и присвоение значения
  int j;
  int q;
  j = stroka.indexOf("success");
  q = stroka.indexOf("_(]AdIXW");
  if ((q >= 0) && (j >= 0))
  {
    Serial.println("W28=" + String(millis()));
    return true;
  }
  else
  {
    // Неудача в обновлении данных на сервере
    Serial.println("W27=" + String(millis()));
    return false;
  }
}

// Функция отправки на сервер данных об инкассации
boolean otprInkass()
{
  int z = 0;
  stroka = stroka0 + "U=" + numApp + "&T=" + kluchPolzov + "&I=" + bb + P4;
  stroka += "&NN=" + String(NNNN) + "&upd=" + String(upd);

  Serial.println("W14=" + stroka);

  memset(msg, '\0', 1100);
  numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

  Serial.println("\nW15: " + String(numdata));
  Serial.println("\nW16:");
  Serial.println("<" + stroka + ">");

  if (numdata == 0)     // Ответ пустой
  {
    // Неудача в обновлении данных на сервере
    Serial.println("W29=" + String(millis()));
    return false;
  }

  // Анализ ответа от сервера
  stroka = "";
  z = strlen(msg);
  for (int i = z - 20; i < numdata; i++) stroka += msg[i];

  // Разбор ответного сообщения и присвоение значения
  int j;
  int q;
  j = stroka.indexOf("success");
  q = stroka.indexOf("_(]AdIXW");
  if ((q >= 0) && (j >= 0))
  {
    Serial.println("W30=" + String(millis()));
    return true;
  }
  else
  {
    Serial.println("29=" + String(millis()));
    return false;
  }
}

// Функция отправки на сервер данных о клиенте без TM
boolean otprNalich()
{
  int z = 0;
  stroka = stroka0 + "U=" + numApp + "&N=" + String(avansClient);
  stroka += "&VN=" + String(literClient) + "&SN=" + String(literClient * cenaLitr);
  stroka += "&m=" + String(itogo) + P5 + "&NN=" + String(NNNN) + "&upd=" + String(upd);

  Serial.println("W14=" + stroka);

  memset(msg, '\0', 1100);
  numdata = inet.httpGET(serverIP.c_str(), 87, stroka.c_str(), msg, 1100);

  Serial.println("\nW15: " + String(numdata));
  Serial.println("\nW16:");
  Serial.println("<" + stroka + ">");

  if (numdata == 0)     // Ответ пустой
  {
    // Неудача в обновлении данных на сервере
    Serial.println("W31=" + String(millis()));
    return false;
  }

  // Анализ ответа от сервера
  stroka = "";
  z = strlen(msg);
  for (int i = z - 20; i < numdata; i++) stroka += msg[i];

  // Разбор ответного сообщения и присвоение значения
  int j;
  int q;
  j = stroka.indexOf("success");
  q = stroka.indexOf("_(]AdIXW");
  if ((q >= 0) && (j >= 0))
  {
    Serial.println("W32=" + String(millis()));
    return true;
  }
  else
  {
    // Неудача в обновлении данных на сервере
    Serial.println("W31=" + String(millis()));
    return false;
  }
}

//========================= Работа с папкой Client ====================================
void rabotaClient()
{
  byte data;
  String nameClient = "";
  File dClient = SD.open(pathClient);
  while (true)
  {
    File fClient = dClient.openNextFile();
    if (!fClient) break;                           // Выход из цикла, если больше нет файлов
    // Попытка отправки данных о клиенте на сервер. Удаление файла, если успех
    nameClient = fClient.name();
    Serial.print(nameClient);
    Serial.print("  Kluch=");
    fClient.seek(0);                               // На начало файла
    data = fClient.read();                          // Пропуск нулевого байта ключа
    Serial.print(data, HEX);
    Serial.print(" ");
    fClient.read();                                // Пропуск пробела

    kluchPolzov = "";
    for (int i = 1; i < 8; i++)
    {
      data = fClient.read();
      Serial.print(data, HEX);
      Serial.print(" ");
      kluchPolzov += data;
      fClient.read();                              // Пропуск пробела
    }
    fClient.read();                                // Пропуск пробела

    bb = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      bb += abc;
    }
    Serial.print(" Balans=" + bb);
    fClient.read();                                // Пропуск байта 0x0D
    fClient.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      hh += abc;
    }
    literClient = hh.toInt();
    Serial.print(" Liters=" + hh);
    fClient.read();                                // Пропуск байта 0x0D
    fClient.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      hh += abc;
    }
    avansClient = hh.toInt();
    Serial.print(" Popoln=" + hh);
    fClient.read();                                // Пропуск байта 0x0D
    fClient.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      hh += abc;
    }
    itogo = hh.toInt();
    Serial.print(" Itogo=" + hh);
    fClient.read();                                // Пропуск байта 0x0D
    fClient.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      hh += abc;
    }
    NNNN = hh.toInt();                             // Номер посылки
    Serial.print(" NNNN=" + hh);
    fClient.read();                                // Пропуск байта 0x0D
    fClient.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fClient.peek() != 0x0D)
    {
      abc = fClient.read();
      hh += abc;
    }
    upd = hh.toInt();                              // Флаг посылки
    Serial.println(" upd=" + hh);
    fClient.close();                               // Закрытие файла

    otvetOtpr = otprClient();                      // true - успешная отправка
    if (otvetOtpr)
    {
      SD.remove(pathClient + nameClient);          // Удаление файла
    }
  }
  dClient.close();
}

//========================= Работа с папкой Inkass ====================================
void rabotaInkass()
{
  byte data;
  String nameInkass = "";
  File dInkass = SD.open(pathInkass);
  while (true)
  {
    File fInkass = dInkass.openNextFile();
    if (!fInkass) break;                           // Выход из цикла, если больше нет файлов
    // Попытка отправки данных о клиенте на сервер. Удаление файла, если успех
    nameInkass = fInkass.name();
    Serial.print(nameInkass);
    Serial.print("  Kluch=");
    fInkass.seek(0);
    data = fInkass.read();                         // Пропуск нулевого байта ключа
    Serial.print(data, HEX);
    Serial.print(" ");
    fInkass.read();                                // Пропуск пробела

    kluchPolzov = "";
    for (int i = 1; i < 8; i++)
    {
      data = fInkass.read();
      Serial.print(data, HEX);
      Serial.print(" ");
      kluchPolzov += data;
      fInkass.read();                              // Пропуск пробела
    }
    fInkass.read();                                // Пропуск пробела

    bb = "";
    while (fInkass.peek() != 0x0D)
    {
      abc = fInkass.read();
      bb += abc;
    }
    Serial.print("  Sum_ink=" + bb);
    fInkass.read();                                // Пропуск байта 0x0D
    fInkass.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fInkass.peek() != 0x0D)
    {
      abc = fInkass.read();
      hh += abc;
    }
    NNNN = hh.toInt();                             // Номер посылки
    Serial.print(" NNNN=" + hh);
    fInkass.read();                                // Пропуск байта 0x0D
    fInkass.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fInkass.peek() != 0x0D)
    {
      abc = fInkass.read();
      hh += abc;
    }
    upd = hh.toInt();                              // Флаг посылки
    Serial.print(" upd=" + hh);
    fInkass.close();                               // Закрытие файла

    otvetOtpr = otprInkass();                      // true - успешная отправка
    if (otvetOtpr)
    {
      SD.remove(pathInkass + nameInkass);          // Удаление файла
    }
  }
  dInkass.close();
}

//========================= Работа с файлом Nalich.txt ================================
void rabotaNalich()
{
  if (SD.exists("Nalich.txt"))
  {
    File fNalich = SD.open("Nalich.txt");
    fNalich.seek(0);
    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    avansClient = hh.toInt();            // Аванс клиента
    Serial.print("Nal.txt:  Nalich=" + hh);
    fNalich.read();                                // Пропуск байта 0x0D
    fNalich.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)       // Пропуск байта 0x0D
    {
      abc = fNalich.read();
      hh += abc;
    }
    literClient = hh.toInt();            // Литры клиента
    Serial.print(" Liters=" + hh);
    fNalich.read();                                // Пропуск байта 0x0D
    fNalich.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)       // Пропуск байта 0x0D
    {
      abc = fNalich.read();
      hh += abc;
    }
    itogo = hh.toInt();                  // Копилка
    Serial.print(" Itogo=" + hh);
    fNalich.read();                                // Пропуск байта 0x0D
    fNalich.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    NNNN = hh.toInt();                             // Номер посылки
    Serial.print(" NNNN=" + hh);
    fNalich.read();                                // Пропуск байта 0x0D
    fNalich.read();                                // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    upd = hh.toInt();                             // Флаг посылки
    Serial.println(" upd=" + hh);
    fNalich.close();                               // Закрытие файла

    otvetOtpr = otprNalich();                      // true - успешная отправка
    if (otvetOtpr)
    {
      SD.remove("Nalich.txt");                     // Удаление файла
    }
  }
}

//==================== Поиск в папке Client файла с прислоненным ключом ===================
String poiskClient(String kluchPolzov)             // true - файл найден, false - файла нет
{
  byte data;
  String kluchClient;
  priznClient = false;
  File dClient = SD.open(pathClient);
  while (true)
  {
    File fClient = dClient.openNextFile();
    if (!fClient) break;                           // Выход из цикла, если больше нет файлов

    fClient.seek(0);                               // На начало файла
    fClient.read();                                // Пропуск нулевого байта ключа
    fClient.read();                                // Пропуск пробела

    kluchClient = "";
    for (int i = 1; i < 8; i++)
    {
      data = fClient.read();
      kluchClient += data;
      fClient.read();                              // Пропуск пробела
    }

    if (kluchClient == kluchPolzov)                // Найден файл
    {
      hh = fClient.name();
      priznClient = true;
      fClient.close();                             // Закрытие файла
      dClient.close();
      return hh;                                   // Возврат имени файла клиента
    }
    else fClient.close();                          // Закрытие файла
  }
  dClient.close();
  return "";                                       // Возврат пустой строки
}

//==================== Поиск в папке Inkass файла с прислоненным ключом ===================
String poiskInkass(String kluchPolzov)             // true - файл найден, false - файла нет
{
  byte data;
  String kluchInkass;
  priznInkass = false;
  File dInkass = SD.open(pathInkass);
  while (true)
  {
    File fInkass = dInkass.openNextFile();
    if (!fInkass) break;                           // Выход из цикла, если больше нет файлов

    fInkass.seek(0);                               // На начало файла
    fInkass.read();                                // Пропуск нулевого байта ключа
    fInkass.read();                                // Пропуск пробела
    kluchInkass = "";
    for (int i = 1; i < 8; i++)
    {
      data = fInkass.read();
      kluchInkass += data;
      fInkass.read();                              // Пропуск пробела
    }

    if (kluchInkass == kluchPolzov)                // Найден файл
    {
      hh = fInkass.name();
      priznInkass = true;
      fInkass.close();                             // Закрытие файла
      dInkass.close();
      return hh;                                   // Возврат имени файла инкассатора
    }
    else fInkass.close();                          // Закрытие файла
  }
  dInkass.close();
  return "";                                       // Возврат пустой строки
}


long NNNNrabota()
{
  // Чтение уникального номера запроса из NNNN.txt
  fNNNN.seek(0);                                  // На начало файла
  hh = "";
  while (fNNNN.peek() != 0x0D)
  {
    abc = fNNNN.read();
    hh += abc;
  }
  NNNN  = hh.toInt();            // Уникальный номер отправленного ранее запроса
  NNNN++;                        // Добавление 1 к NNNNN
  fNNNN.seek(0);                                  // На начало файла
  fNNNN.println(NNNN);
  fNNNN.flush();

  return NNNN;
}

// Действия с наличкой
void Nalichka()
{
  long avansCl;
  long literCl;
  // Проверка наличия файла Nalich.txt
  if (SD.exists("Nalich.txt"))           // Файл существует
  {
    // Чтение данных файла
    File fNalich = SD.open("Nalich.txt", FILE_WRITE);
    fNalich.seek(0);
    hh  = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    avansCl = hh.toInt() + literClient * cenaLitr;
    fNalich.read();                      // Пропуск байта 0x0D
    fNalich.read();                      // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    literCl = hh.toInt() + literClient;
    fNalich.read();                      // Пропуск байта 0x0D
    fNalich.read();                      // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    fNalich.read();                     // Пропуск байта 0x0D
    fNalich.read();                     // Пропуск байта 0x0A

    hh = "";
    while (fNalich.peek() != 0x0D)
    {
      abc = fNalich.read();
      hh += abc;
    }
    NNNN = hh.toInt();                  // Номер посылки

    // Сохранение данных в файл Nalich.txt
    fNalich.seek(0);
    fNalich.println(avansCl);
    fNalich.println(literCl);
    literClient = 0;                    // Обнуление литров
    itogo = SummaKop();                 // + сохранение в SD, очистка счетчиков
    fNalich.println(itogo);
    fNalich.println(NNNN);
    fNalich.println(1);
    fNalich.flush();
    fNalich.close();

    Serial.println("W38");
  }
  else
  { // Файл НЕ существует
    NNNN = NNNNrabota();                // Получение уникального номера запроса
    File fNalich = SD.open("Nalich.txt", FILE_WRITE);
    fNalich.seek(0);
    fNalich.println(literClient * cenaLitr); // Аванс
    fNalich.println(literClient);            // Литры
    literClient = 0;                         // Обнуление литров
    itogo = SummaKop();                      // + сохранение в SD, очистка счетчиков
    fNalich.println(itogo);                  // Копилка
    fNalich.println(NNNN);                   // Номер посылки
    fNalich.println(0);                      // upd = 0
    fNalich.flush();
    fNalich.close();

    Serial.println("W39");
  }
}

long SummaKop()
{
  long itogo;
  fCoin.seek(0);                          // На начало файла
  co = "";
  while (fCoin.peek() != 0x0D)
  {
    abc = fCoin.read();
    co += abc;
  }
  coinItog = co.toInt();
  if (pulse_coin > 0)                     // Есть внесенные монеты
  {
    coinItog += pulse_coin;
    pulse_coin  = 0;                      // Обнуление счетчика импульсов монет
    fCoin.seek(0);                        // На начало файла
    fCoin.println(coinItog);              // Обновление файла Coin.txt
    fCoin.flush();
  }
  fBanknote.seek(0);                                  // На начало файла
  ba = "";
  while (fBanknote.peek() != 0x0D)
  {
    abc = fBanknote.read();
    ba += abc;
  }
  banknoteItog = ba.toInt();
  if (pulse_banknote > 0)                 // Есть внесенные купюры
  {
    banknoteItog += 10 * pulse_banknote;
    pulse_banknote = 0;                   // Обнуление счетчика импульсов купюр
    fBanknote.seek(0);                    // На начало файла
    fBanknote.println(banknoteItog);      // Обновление файла Banknote.txt
    fBanknote.flush();
  }
  itogo = coinItog + banknoteItog;
  return itogo;
}

