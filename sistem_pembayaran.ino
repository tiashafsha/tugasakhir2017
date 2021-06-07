#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Ethernet.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "192.168.43.25";  // IPLAPTOP
IPAddress ip(192, 168, 43, 87); //IP ARDUINO
IPAddress gateway(192, 168, 43, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetClient client;  

char customKey;
bool recharge = true;
bool state = true;
bool cek = true;

#define RST_PIN         5
#define SS_PIN          53
#define buzzer          40
#define Redled          42
#define Greenled        44

MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

long bayarparu = 400;
long bayarumum = 200;
long bayarjantung = 700;
long bayar;
bool isiSaldo = false;
bool notif = true;
String input;

int digit;

const byte rows = 4;
const byte columns = 4;

String pengobatan;
int a;
//keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Initializing pins for keypad
byte row_pins[rows] = {38, 36, 34, 32};
byte column_pins[columns] = {30, 28, 26, 24};

// Create instance for keypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

long saldo;
long jumlahtopup;

long OLDsaldo;
int OLDdigit;

void setup() {
    Serial.begin(9600);
    SPI.begin();    
    pinMode(buzzer, OUTPUT);
    pinMode(Greenled, OUTPUT); 
    pinMode(Redled, OUTPUT);    
    mfrc522.PCD_Init(); 

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("Alat Pembayaran Rumah Sakit");
    Serial.println();
    Serial.println("Peringatan : Data akan di simpan pada RFID Card pada sector #1 (blocks #4)");
    Serial.println();
    Serial.println();
  //    if (Ethernet.begin(mac) == 0) {
    //    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip, gateway, subnet);
     // }
    //delay(1000);
    Serial.println("server is at: ");
    Serial.println(Ethernet.localIP());
    Serial.println("Menghubungkan");

    lcd.init();
    lcd.backlight();
    lcd.print("Modul Pembayaran");  
    lcd.setCursor(3,1);
    lcd.print("Rumah Sakit");  
    delay(4000);
}

void loop(){
  if (recharge == 0){
      isi();
  }
    else if (state == 0){
      bayar1();
      bayar2();
      bayar3();
    }
  else if (cek == 0){
      ceksaldo();
    }
  else
  {
    lcd.setCursor(0,0);
    lcd.print("Menu:    A.Isi  ");
    lcd.setCursor(0,1);
    lcd.print("B.Bayar  C.Cek  ");
    KeyPad();
  }
}

void ceksaldo(){
     if (notif){
        notif = false;
        lcd.print("Tap Kartu Anda");
        delay(3000);
        lcd.clear();
      }
      if ( ! mfrc522.PICC_IsNewCardPresent()){
          return;
      }
    
      if ( ! mfrc522.PICC_ReadCardSerial()){
          return;
      }
    
      Serial.println();
      Serial.print("Card UID:");
      String content= "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
         Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
         Serial.print(mfrc522.uid.uidByte[i], HEX);
         content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
         content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase(); 
      Serial.println();
      
      Serial.println();
      Serial.print("Tipe Kartu : ");
      MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
      Serial.println(mfrc522.PICC_GetTypeName(piccType));
    
      if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
          &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
          &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
          Serial.println(F("Kode ini hanya dapat digunakan pada MIFARE Classic cards 1KB - 13.56MHz."));
          notif = true;
          delay(2000);
          resetReader1();
          return;
      }
      byte sector         = 1;
      byte blockAddr      = 4;
      
      MFRC522::StatusCode status;
      byte buffer[18];
      byte size = sizeof(buffer);
    
      mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
      Serial.println();
    
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
          Serial.println("Gagal Baca Kartu RFID");
          resetReader1();
           state=1;
          return;
      }
      OLDdigit = buffer[0];
      OLDsaldo = OLDdigit;
      OLDsaldo *= 100;
      Serial.print("Saldo Kartu Sekarang : ");
      Serial.println(OLDsaldo);
      Serial.println();
      lcd.setCursor(0,0);
      lcd.print("Saldo Kartu");
      lcd.setCursor(0,1);
      lcd.print(OLDsaldo);
      delay(4000);
      lcd.clear();
      cek=1;
}

void isi(){
    
    isiSaldo = true;
    input = "";
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Input Nominal");
    lcd.setCursor(2,1);
    lcd.print("dan Tap Kartu");
    delay(4000);
    input += GetNumber();
    saldo = input.toInt();
    digit = saldo;
    saldo *= 100;
    jumlahtopup=saldo;
    lcd.clear();
    
    Serial.print("saldo yang di input : ");
    lcd.print("saldo input: ");
    lcd.setCursor(0,1);
    lcd.print(saldo);
    Serial.println(saldo);
    delay(3000);
    lcd.clear();
    
  
  if ( ! mfrc522.PICC_IsNewCardPresent()){
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()){
      return;
  }

  Serial.println();
  Serial.print("Card UID:");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase(); 
  Serial.println();
  
  Serial.print("Tipe Kartu : ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Kode ini hanya dapat digunakan pada MIFARE Classic cards 1KB - 13.56MHz."));
      notif = true;
      delay(2000);
      resetReader();
      return;
  }
  byte sector         = 1;
  byte blockAddr      = 4;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  if (isiSaldo){
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");

        resetReader();
        return;
    }
    OLDdigit = buffer[0];
    OLDsaldo = OLDdigit;
    OLDsaldo *= 100;
    Serial.print("Saldo Kartu Sebelumnya : ");
    Serial.println(OLDsaldo);
    Serial.println();
    saldo += OLDsaldo;
    digit += OLDdigit;
    
    if (digit > 255){
      saldo = 0;
      digit = 0;
      Serial.println("Saldo sebelum di tambah saldo baru melebihi 255 ribu");
      Serial.println("Gagal tambah saldo");
      resetReader();
      return;
    }
    
    byte dataBlock[]    = {
        digit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Write Saldo pada Kartu RFID");
    }
    Serial.println();
  
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
    }

    Serial.println();
  
    Serial.println("Menambahkan Saldo...");
    if (buffer[0] == dataBlock[0]){
      Serial.print("Saldo kartu sekarang : ");
      Serial.println(saldo);
      lcd.print("Saldo kartu: ");
      lcd.setCursor(0,1);
      lcd.print(saldo);
      delay(2000);
      lcd.clear();
      if (content.substring(1) == "53 86 EB 27"){
         if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              Serial.println(server);
              client.print("GET /php/isi.php?nama_pasien=Tias%20Hafsha&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Tias%20Hafsha&topup_saldo=");
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "AA B1 54 3C"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Arista%20Puri&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Arista%20Puri&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "6A 1C F9 3A"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Agnes%20Christi&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Agnes%20Christi&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "5A 30 C3 3A"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Rahma%20Kamila&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Rahma%20Kamila&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "5A B9 3B 3B"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Maulana%20Yuliati&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Maulana%20Yuliati&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "BB 1B E8 00"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Irene%20Christovita&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Irene%20Christovita&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "05 C4 CE 2D"){
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/isi.php?nama_pasien=Muhammad%20Farhan&topup_saldo="); //nama pasien diganti
              Serial.print("GET /php/isi.php?nama_pasien=Muhammad%20Farhan&topup_saldo="); //nama pasien diganti
              client.print(jumlahtopup);
              Serial.println(jumlahtopup);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      Serial.println("_ Berhasil isi saldo pada kartu ___");
    }else{
      Serial.println("------------ GAGAL ISI SALDO --------------");
    }
  }else{
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        saldo = 0;
        digit = 0;
        resetReader();
        return;
    }
    Serial.println(); 
  }

  saldo = 0;
  digit = 0;

  Serial.println();
  Serial.println();

  resetReader();
  lcd.clear();
  recharge=1;
}

void KeyPad(){
  customKey = customKeypad.getKey();
  
  if (customKey)
  {
    if (customKey == 'A')
    {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Mode Top Up");
          lcd.setCursor(0, 1);
          lcd.print("................");
          delay(1500);
          lcd.clear();
          recharge = 0;
    }
    
    if (customKey == 'B')
    {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Mode Bayar");
          lcd.setCursor(0, 1);
          lcd.print("................");
          delay(1500);
          spesialis();
    }
    if (customKey == 'C')
    {
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("Mode Cek Saldo");
          lcd.setCursor(0, 1);
          lcd.print("................");
          delay(1500);
          cek=0;
    }
  }
}



void Rfid(){
 lcd.clear();
 if (notif){
    notif = false;
    lcd.print("Tap Kartu Anda");
    lcd.clear();
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()){
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()){
      return;
  }

  Serial.println();
  Serial.print("Card UID:");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase(); 
  Serial.println();
  
  Serial.println();
  Serial.print("Tipe Kartu : ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Kode ini hanya dapat digunakan pada MIFARE Classic cards 1KB - 13.56MHz."));
      notif = true;
      delay(2000);
      resetReader1();
      return;
  }
  byte sector         = 1;
  byte blockAddr      = 4;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("Gagal Baca Kartu RFID");
      resetReader1();
       state=1;
      return;
  }
  OLDdigit = buffer[0];
  OLDsaldo = OLDdigit;
  OLDsaldo *= 100;
  Serial.print("Saldo Kartu Sebelumnya : ");
  Serial.println(OLDsaldo);
  Serial.println();
  lcd.setCursor(0,0);
  lcd.print("Saldo Kartu");
  lcd.setCursor(0,1);
  lcd.print(OLDsaldo);
  delay(3000);
  

  if (OLDdigit < digit){
    digitalWrite(buzzer, HIGH);
    digitalWrite(Redled, HIGH);
    lcd.setCursor(0,0);
    lcd.print("Gagal Bayar");
    lcd.setCursor(0,1);
    lcd.print("Saldo Kurang");
    delay(4000);
    digitalWrite(buzzer, LOW);
    digitalWrite(Redled, LOW);
    resetReader1();
     state=1;
    return;
  }

  OLDdigit -= digit;
  
  byte dataBlock[]    = {
      
      OLDdigit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("Gagal Write Saldo pada Kartu RFID");
  }
  Serial.println();

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.println("Gagal Baca Kartu RFID");
       state=1;
  }

  Serial.println();
  
  if (buffer[0] == dataBlock[0]){
    Serial.println("Mengurangi Saldo...");  
    saldo = buffer[0];
    saldo *= 100;
    digitalWrite(buzzer, HIGH);
    digitalWrite(Greenled, HIGH);
    lcd.setCursor(0,0);
    Serial.println("Transaksi Berhasil");
    lcd.print("Berhasil Bayar");
    lcd.setCursor(0,1);
    lcd.print("Sisa Saldo ");
    lcd.print(saldo);
    if (content.substring(1) == "53 86 EB 27"){
         Serial.println("Tias Hafsha");
         if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Tias%20Hafsha&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Tias%20Hafsha&pembayaran_pengobatan=");
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "AA B1 54 3C"){
        Serial.println("Arista");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Arista%20Puri&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Arista%20Puri&pembayaran_pengobatan="); //nama pasien diganti
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "6A 1C F9 3A"){
        Serial.println("Agnes");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Agnes%20Christi&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Agnes%20Christi&pembayaran_pengobatan="); //nama pasien diganti
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "5A 30 C3 3A"){
        Serial.println("Mila");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Rahma%20Kamila&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Rahma%20Kamila&pembayaran_pengobatan="); //nama pasien diganti
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "5A B9 3B 3B"){
        Serial.println("Maulana");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Maulana%20Yuliati&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Maulana%20Yuliati&pembayaran_pengobatan="); //nama pasien diganti
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "BB 1B E8 00"){
        Serial.println("Irene");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Irene%20Christovita&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Irene%20Christovita&pembayaran_pengobatan="); //nama pasien diganti
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
      else if (content.substring(1) == "05 C4 CE 2D"){
        Serial.println("M Farhan");
        if (client.connect(server, 80)) {
              Serial.println("Terhubung");
              client.print("GET /php/bayar.php?nama_pasien=Muhammad%20Farhan&pembayaran_pengobatan="); //nama pasien diganti
              Serial.print("GET /php/bayar.php?nama_pasien=Muhammad%20Farhan&pembayaran_pengobatan=");
              client.print(pengobatan);
              Serial.print(pengobatan);
              client.print("&pemasukan=");
              Serial.print("&pemasukan=");
              client.print(bayar);
              Serial.print(bayar);
              client.print(" ");     
              client.print("HTTP/1.1");
              client.println();
              client.print("Host: ");
              client.println(server);
              client.println("Connection: close");
              client.println();}
            else {
              Serial.println("Koneksi gagal");
            }
      }
    delay(2000);
    digitalWrite(buzzer, LOW);
    digitalWrite(Greenled, LOW);
    Serial.println("------------ BERSHASIL bayar Tagihan --------------");
    state=1;
  }else{
    Serial.println("------------ GAGAL bayar Tagihan --------------");
     state=1;
  }
  Serial.println();
  Serial.println();
  resetReader1();
}

void spesialis(){
  lcd.clear();
  lcd.setCursor(0,0);
   lcd.print("Pilih Spesialis");
   delay(2000);
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("1.Umum   2.Paru");
   lcd.setCursor(3,1);
   lcd.print("3.Jantung");
   delay(4000);
   lcd.clear();
   lcd.setCursor(1, 0);
   lcd.print("Masukkan Kode");
   delay(2000);
   state = 0;
}

//void dump_byte_array(byte *buffer, byte bufferSize) {    
//}

void resetReader(){
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  notif = true;
  isiSaldo = false;
}

void resetReader1(){
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  notif = true;
}

int GetNumber()
{
   lcd.clear();
   lcd.print("*:hapus #:enter"); 
   delay(4000);
   lcd.clear();
   
   int num = 0;
   char key = customKeypad.getKey();
   while(key != '#')
   {
      switch (key)
      {  
         case NO_KEY:
            break;
  
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            lcd.print(key);
            num = num * 10 + (key - '0');
            break;

         case '*':
            num = 0;
            lcd.clear();
            break;
      }

      key = customKeypad.getKey();
   }

   return num;
}

void bayar1(){
  customKey = customKeypad.getKey();
{
  if (customKey)
  {
    if (customKey == '1')
    { 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Pembayaran Umum");
          lcd.setCursor(0,1);
          lcd.print("Sebesar: ");
          lcd.print(bayarumum);
          delay(4000);
          lcd.clear();
          bayar=bayarumum;
          pengobatan= "Umum";
          digit = bayarumum/100;
          Rfid();
    }
  }
 }
}

void bayar2(){
  customKey = customKeypad.getKey();
{
  if (customKey)
  {
    if (customKey == '2')
    { 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Pembayaran Paru");
          lcd.setCursor(0,1);
          lcd.print("Sebesar: ");
          lcd.print(bayarparu);
          delay(4000);
          lcd.clear();
          bayar=bayarparu;
          pengobatan= "Paru";
          digit = bayarparu/100;
          Rfid();
    }
  }
 }
}

void bayar3(){
  customKey = customKeypad.getKey();
{
  if (customKey)
  {
    if (customKey == '3')
    { 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Pembayaran Jantung");
          lcd.setCursor(0,1);
          lcd.print("Sebesar: ");
          lcd.print(bayarjantung);
          delay(4000);
          lcd.clear();
          bayar=bayarjantung;
          pengobatan= "Jantung";
          
          digit = bayarjantung/100;
          Rfid();
    }
  }
 }
}
