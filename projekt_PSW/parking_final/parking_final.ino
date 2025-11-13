#include <ESP32Servo.h> // Biblioteka do obsługi serwa dla ESP32
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definicje pinów dla czujników HC-SR04
const int trigPin1 = 19;  // Trig pierwszego czujnika (przed szlabanem)
const int echoPin1 = 23;  // Echo pierwszego czujnika (przed szlabanem)
const int trigPin2 = 5;   // Trig drugiego czujnika (za szlabanem)
const int echoPin2 = 18;  // Echo drugiego czujnika (za szlabanem)

// Ustawienia serwomechanizmu
Servo szlaban;
const int servoPin = 13;

// Ustawienia wyświetlacza OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Parametry parkingu
const int maxMiejsca = 3; // Maksymalna liczba miejsc
int wolneMiejsca = maxMiejsca; // Liczba wolnych miejsc

void setup() {
  // Konfiguracja pinów czujników
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Inicjalizacja serwa
  szlaban.attach(servoPin, 500, 2400); // Pin, minimalny impuls, maksymalny impuls
  szlaban.write(0); // Szlaban w pozycji poziomej (zamkniętej)

  // Inicjalizacja wyświetlacza
  if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
    Serial.println(F("Błąd inicjalizacji OLED"));
    for (;;); // Zatrzymaj program w razie błędu
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Inicjalizacja monitora szeregowego
  Serial.begin(115200);
  displayStatus();
}

void loop() {
  // Sprawdzanie pierwszego czujnika (przed szlabanem - wjazd)
  if (detectCar(trigPin1, echoPin1)) {
    if (wolneMiejsca > 0) { // Jeśli są wolne miejsca
      Serial.println("Auto wykryte przed szlabanem. Otwieram szlaban...");
      otworzSzlaban(); // Otwórz szlaban

      // Poczekaj, aż auto przejedzie przez drugi czujnik
      bool autoPrzejechalo = false;
      unsigned long startTime = millis(); // Licz czas
      while (millis() - startTime < 10000) { // Czekaj maksymalnie 10 sekund
        if (detectCar(trigPin2, echoPin2)) {
          autoPrzejechalo = true;
          break;
        }
      }

      if (autoPrzejechalo) {
        Serial.println("Auto przejechało przez szlaban.");
        wolneMiejsca--; // Zmniejsz liczbę wolnych miejsc
        displayStatus(); // Zaktualizuj wyświetlacz
      } else {
        Serial.println("Nie wykryto auta za szlabanem.");
      }

      zamknijSzlaban(); // Zamknij szlaban po przejechaniu auta
    } else {
      Serial.println("Brak wolnych miejsc. Szlaban nie otwiera się.");
      displayNoSpace(); // Wyświetl komunikat "Brak wolnych miejsc!" na 5 sekund
      delay(5000); // Opóźnienie przed kolejną pętlą
      displayStatus(); // Przywróć normalny komunikat na wyświetlaczu
    }
  }

  // Sprawdzanie drugiego czujnika (za szlabanem - wyjazd)
  if (detectCar(trigPin2, echoPin2)) {
    Serial.println("Auto wykryte przed wyjazdem. Otwieram szlaban...");
    otworzSzlaban(); // Otwórz szlaban

    // Poczekaj, aż auto przejedzie przez pierwszy czujnik
    bool autoWyjechalo = false;
    unsigned long startTime = millis(); // Licz czas
    while (millis() - startTime < 10000) { // Czekaj maksymalnie 10 sekund
      if (detectCar(trigPin1, echoPin1)) {
        autoWyjechalo = true;
        break;
      }
    }

    if (autoWyjechalo) {
      Serial.println("Auto wyjechało z parkingu.");
      if (wolneMiejsca < maxMiejsca) { // Unikaj przekraczania maksymalnej liczby miejsc
        wolneMiejsca++; // Zwiększ liczbę wolnych miejsc
        displayStatus(); // Zaktualizuj wyświetlacz
      }
    } else {
      Serial.println("Nie wykryto auta wyjeżdżającego przez pierwszy czujnik.");
    }

    zamknijSzlaban(); // Zamknij szlaban po wyjeździe auta
  }

  delay(100); // Krótka przerwa w pętli
}

// Funkcja do mierzenia odległości (w cm)
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH); // Czas trwania sygnału w mikrosekundach
  float distance = (duration * 0.0343) / 2; // Oblicz odległość w cm
  return distance;
}

// Funkcja do wykrywania auta
bool detectCar(int trigPin, int echoPin) {
  float distance = measureDistance(trigPin, echoPin);
  return (distance > 0 && distance < 5); // Wykrywanie w zakresie <5 cm
}

// Funkcja otwierająca szlaban
void otworzSzlaban() {
  for (int angle = 0; angle <= 90; angle++) { // Od 0° do 90°
    szlaban.write(angle);
    delay(10); // Opóźnienie między krokami
  }
  Serial.println("Szlaban otwarty.");
}

// Funkcja zamykająca szlaban
void zamknijSzlaban() {
  for (int angle = 90; angle >= 0; angle--) { // Od 90° do 0°
    szlaban.write(angle);
    delay(10); // Opóźnienie między krokami
  }
  Serial.println("Szlaban zamknięty.");
}

// Funkcja do aktualizacji wyświetlacza OLED
void displayStatus() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(2); // Powiększony tekst
  display.print("Miejsca ");
  display.setCursor(0, 30);
  display.print("wolne: ");
  display.print(wolneMiejsca);
  display.print("/");
  display.print(maxMiejsca);
  display.display();
  Serial.print("Miejsca wolne: ");
  Serial.print(wolneMiejsca);
  Serial.print("/");
  Serial.println(maxMiejsca);
}

// Funkcja wyświetlająca komunikat "Brak wolnych miejsc!"
void displayNoSpace() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(2); // Powiększony tekst
  display.print("Brak");
  display.setCursor(0, 30);
  display.print("wolnych");
  display.setCursor(0, 50);
  display.print("miejsc!");
  display.display();
  Serial.println("Brak wolnych miejsc!");
}