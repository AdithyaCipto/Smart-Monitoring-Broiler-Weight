#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HX711_ADC.h>
#include <ESP32_Servo.h>

const int HX711_dout = 16;  // mcu > HX711 dout pin
const int HX711_sck = 4;    // mcu > HX711 sck pin

const char* ssid = "My_Cipttt";
const char* password = "kedaireka";

const char* serverUrl = "https://script.google.com/macros/s/AKfycbxxtVnLR6rHuk8joPssc5Ep9WmozIH7-NDG1kR6qbHbjHXq2dMA8FVEXMs9UGl7J34n/exec";  // URL server tempat Anda ingin mengirim data

HX711_ADC LoadCell(HX711_dout, HX711_sck);
unsigned long t = 0;
int stableCount = 0;
int lastStableGram = 0;

int GRAM;
int berat;
int pir = 32;
Servo myservo1;
Servo myservo2;
Servo myservo3;
Servo myservo4;
int a = 0;
int z = 180;

void sendWeightData(int weightGram);

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Starting...");


  float calibrationValue = 22.11;  // Set calibration value here

  LoadCell.begin();
  LoadCell.start(2000);                     // starting the load cell without tare
  LoadCell.setCalFactor(calibrationValue);  // set calibration factor (float)
  Serial.println("Startup is complete");

  while (!LoadCell.update())
    ;
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settling time ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  } else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi");

  pinMode(pir, INPUT);
  myservo1.attach(14);
  myservo2.attach(13);
  myservo3.attach(12);
  myservo4.attach(15);
}
//------------------
float beratSebelumnya = 0;  // Berat sebelumnya untuk mendeteksi perubahan
float beratStabil = 0;      // Berat yang dianggap stabil
int jumlahStabil = 0;            // Menghitung jumlah pembacaan stabil berturut-turut
int jumlahStabilDiperlukan = 5;  // Jumlah pembacaan stabil yang dibutuhkan
float ambangPerubahan = 3.0;
bool dataDitampilkan = false; // Menyimpan status apakah data sudah ditampilkan
//---------------

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 100;  // increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) {
    newDataReady = true;
  }

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      berat = LoadCell.getData();
      if (berat < 0) {
        berat = 0.00;
      }

      Serial.print("Berat : ");
      Serial.print(berat);
      Serial.println(" gram");
      KirimData(berat);

      // Check for stability
      if (abs(berat - lastStableGram) <= 20) {  // Jika perubahan berat kurang dari 10 gram, anggap stabil
        stableCount++;
      } else {
        stableCount = 0;  // Reset jika berat tidak stabil
        lastStableGram = berat;
      }
      // Variabel untuk pengukuran berat

      if (abs(berat - beratSebelumnya) >= ambangPerubahan) {
        Serial.print("Berat process : ");
        Serial.println(berat);
        dataDitampilkan = false;
      }
      if (stableCount >= jumlahStabilDiperlukan && !dataDitampilkan) {
        beratStabil = berat;
        Serial.print("Berat Stabil : ");
        Serial.println(beratStabil);
        stableCount = 0; 
       dataDitampilkan = true; 

      }
      beratSebelumnya = berat;
      newDataReady = false;
      t = millis();
      delay(250);
    }
  }
  int pirValue = digitalRead(pir);
  if (pirValue == HIGH && berat > 50) {
    Serial.println("Ada Ayam");
    myservo1.write(a);
    myservo2.write(a);
    myservo3.write(z);
    myservo4.write(z);
    delay(250);
  } else {
    Serial.println("Tidak Ada Ayam");
    myservo1.write(z);
    myservo2.write(z);
    myservo3.write(a);
    myservo4.write(a);
    delay(250);
  }
}

void KirimData(int BERAT) {
  // Cek jika terkoneksi ke Wi-Fi
  HTTPClient http;

  // Mulai koneksi dan kirim HTTP header
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Siapkan data yang akan dikirim
  String httpRequestData = "gram=" + String(BERAT);

  // Kirim HTTP POST request
  if (BERAT > 50) {
    Serial.println("Ada Ayam");
    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
  } else {
    Serial.println("Tidak Ada Ayam");
  }
  // Tutup koneksi
  http.end();
}