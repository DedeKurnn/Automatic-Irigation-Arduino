#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750FVI.h>

#define SOIL_SENSOR A0
#define DISTANCE_TRIG 13
#define DISTANCE_ECHO A1  
#define DHTTYPE DHT22
#define DHTPIN 2
#define PUMP 3
#define PIEZO_PIN 9

#define DISTANCE_THRESHOLD 10

// Inisialisasi variabel
int air_temperature = 0;
int air_humidity = 0;
int soil_humidity = 0;
int water_distance = 0;
int light_intensity = 0;
int optimal_soil_humidity_range = 0;
int humidity_difference = 0;
bool sensorValuesChanged = false;

//Inisialisasi servo, LCD, dan DHT22
LiquidCrystal_I2C lcd(0x27, 24, 4);
DHT dht (DHTPIN, DHTTYPE);
BH1750FVI lightMeter(BH1750FVI::k_DevModeContLowRes);

void setup() {
// Menentukan pinmode dari setiap komponen
  Serial.begin(9600);
  Wire.begin();

  pinMode(PIEZO_PIN, OUTPUT);
  pinMode(DISTANCE_TRIG, OUTPUT);
  pinMode(DISTANCE_ECHO, INPUT);
  pinMode(SOIL_SENSOR, INPUT);
  pinMode(PUMP, OUTPUT);
// Inisialisasi servo dan sensor DHT
  dht.begin();
  lightMeter.begin();
  lightMeter.SetMode(0b00010000);
// Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0,0);
  lcd.print("SH:"); //SH: Soil Humidity
  lcd.setCursor(9, 0);
  lcd.print("AH:"); //AH: Air Humidity
  lcd.setCursor(0,1);
  lcd.print("LI:"); //LI: Light Intensity
  lcd.setCursor(9, 1);
  lcd.print("AT:"); //AT: Air Temperature
}

void loop() {
  long watering_duration = 0;
// Mengambil data dari sensor
  int new_air_humidity = dht.readHumidity();
  int new_air_temperature = dht.readTemperature();
  int new_soil_humidity = 100 - (analogRead(SOIL_SENSOR) / 10);
  int new_water_distance = getDistance();
  uint16_t new_light_intensity = lightMeter.GetLightIntensity();

// Menampilkan data pada layar LCD
  lcd.setCursor(3, 0);
  lcd.print(new_soil_humidity);
  lcd.print("%");
  lcd.setCursor(12, 0);
  lcd.print(new_air_humidity);
  lcd.print("%");
  lcd.setCursor(3, 1);
  lcd.print(new_light_intensity);
  lcd.setCursor(12, 1);
  lcd.print(new_air_temperature);
  lcd.print((char)223); //Menampilkan simbol derajat
  lcd.print("C");

// Menampilkan konsol debug
  Serial.print("\nSoil humidity: ");
  Serial.print(new_soil_humidity);
  Serial.print("\n\nAir Temperature: ");
  Serial.print(new_air_temperature);
  Serial.print("\nAir Humidity: ");
  Serial.print(new_air_humidity);

  /* Memeriksa apakah nilai pembacaan sensor berubah dari iterasi sebelumnya, 
   Jika ya, maka perbarui data lama dengan data baru */
  if ((new_soil_humidity != soil_humidity || new_light_intensity != light_intensity || new_air_temperature != air_temperature || new_air_humidity != air_humidity) && new_water_distance < 10) {
    soil_humidity = new_soil_humidity;
    light_intensity = new_light_intensity;
    water_distance = new_water_distance;
    air_temperature = new_air_temperature;
    air_humidity = new_air_humidity;
    sensorValuesChanged = true;
  } else {
    sensorValuesChanged = false;
  }
// Menentukan kelembapan tanah optimal berdasarkan suhu dan kelembapan udara //
  if (new_air_temperature >= 25 && new_air_humidity >= 60) {
    optimal_soil_humidity_range = 80;
  } else if (new_air_temperature >= 20 && new_air_humidity >= 50) {
    optimal_soil_humidity_range = 70;
  } else if (new_air_temperature >= 15 && new_air_humidity >= 40) {
    optimal_soil_humidity_range = 60;
  } else {
    optimal_soil_humidity_range = 50;
  }

// Menampilkan data debug
  Serial.print("\nOptimal soil humidity range: ");
  Serial.print(optimal_soil_humidity_range);
  Serial.print("\nWatering duration (before): ");
  Serial.print(watering_duration);

// Menentukan durasi penyiraman berdasarkan perbedaan kelembapan
  humidity_difference = optimal_soil_humidity_range - new_soil_humidity;

  if (humidity_difference < 10) {
    watering_duration = 0;
  } else if (humidity_difference >= 10 && humidity_difference < 20) {
    watering_duration = 3000;
  } else if (humidity_difference >= 20 && humidity_difference < 30) {
    watering_duration = 4500;
  } else {
    watering_duration = 6000;
  }

// Menampilkan konsol debug
  Serial.print("\nHumidity difference: ");
  Serial.print(humidity_difference);
  Serial.print("\nWatering duration (before adjustment): ");
  Serial.print(watering_duration);

// Menyesuaikan durasi penyiraman berdasarkan intensitas cahaya yang diterima sensor
  if (new_light_intensity < 3000) {
      watering_duration -= 1000;
  } else if (new_light_intensity > 5000) {
      watering_duration += 1000;
  }
  if(watering_duration <= 0) {
    watering_duration = 0;
  }

// Menampilkan konsol debug
  Serial.print("\nWatering duration (after): ");
  Serial.print(watering_duration);

// Memeriksa apakah nilai berubah, jika ya, maka servo akan aktif
  if (sensorValuesChanged) {
    digitalWrite(PUMP, HIGH);
    delay(watering_duration);
    Serial.print(watering_duration);
    digitalWrite(PUMP, LOW);
  }

// Membunyikan buzzer jika air sudah hampir habis
  if (new_water_distance > DISTANCE_THRESHOLD) {
      digitalWrite(PIEZO_PIN, HIGH);
      tone(PIEZO_PIN, 1000);
      delay(500);
      noTone(PIEZO_PIN);
      delay(500);
    }
    delay(2000);
}

// Mendapatkan jarak permukaan air dengan sensor untuk menentukan jumlah air yang tersisa
int getDistance() {
  digitalWrite(DISTANCE_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(DISTANCE_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(DISTANCE_TRIG, LOW);
  
  int duration = pulseIn(DISTANCE_ECHO, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}
