/*
PZEM017
// 18 > A
// 19 > B

PZEM004T
// 16 > TX
// 17 > RX

DHT22
// 26 > OUT
*/

#include <WiFi.h>
#include <ModbusMaster.h>    // ติดตั้งจาก Library Manager ค้นหา ModbusMaster
#include <cynoiot.h>         // ติดตั้งจาก Library Manager ค้นหา cynoiot
#include <PZEM004Tv30.h>     // ติดตั้งจาก Library Manager ค้นหา PZEM004Tv30
#include "DHT.h"             // ติดตั้งจาก Library Manager ค้นหา DHT sensor library

const char ssid[] = "G6PD_2.4G"; //  ชื่อไวไฟ
const char pass[] = "570610193"; //  รหัสผ่านไวไฟ
const char email[] = "timpiga.56789@gmail.com";

// สร้าง object ชื่อ iot จาก class Cynoiot
Cynoiot iot;

// สร้าง object ชื่อ dht จาก class DHT
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
#define DHTPIN 26
DHT dht(DHTPIN, DHTTYPE);

// กำหนดขา RX TX ของ PZEM004T
#define AC_PZEM_RX_PIN 16
#define AC_PZEM_TX_PIN 17

// กำหนดขา RX TX ของ PZEM017
#define DC_PZEM_RX_PIN 18
#define DC_PZEM_TX_PIN 19

// ตั้งค่า shunt -->> 0x0000-100A, 0x0001-50A, 0x0002-200A, 0x0003-300A
uint16_t shuntAddr[] = {0x0001, 0x0001, 0x0000, 0x0000};

// สร้างตัวแปรเก็บค่าไฟฟ้า
float DC_Voltage[4], DC_Current[4], DC_Power[4], DC_Energy[4];
float AC_Voltage[2], AC_Current[2], AC_Power[2], AC_Energy[2], AC_Frequency[2], AC_PF[2];

unsigned long previousMillis = 0;

uint8_t numVariables;  // จำนวนตัวแปรที่ต้องการส่งไปยัง Server
uint16_t updateTime = 3600;  // เวลาอัพเดทค่าไปยัง Server (second)

// ฟังก์ชันตั้งค่าสำหรับการอัพเดทค่าออนไลน์
void iotSetup()
{
  numVariables = 19;
  String *keyname = new String[numVariables]{
      "vd1",
      "id1",
      "pd1",

      "vd2",
      "id2",
      "pd2",

      "vd3",
      "id3",
      "pd3",

      "vd4",
      "id4",
      "pd4",

      "va1",
      "ia1",
      "pa1",

      "va2",
      "ia2",
      "pa2",

      "t"};
  iot.setkeyname(keyname, numVariables);
  // String keyname[2] = {"humid", "temp"};
  // iot.setkeyname(keyname, 2);

  Serial.println("ClinetID:" + String(iot.getClientId()));

  Serial.print("Connecting to server.");
  iot.connect(email);
}

void setup()
{
  // เริ่มต้นการทำงานของ Serial ทั้ง 3 ช่อง
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N2, DC_PZEM_RX_PIN, DC_PZEM_TX_PIN);
  Serial2.begin(9600, SERIAL_8N2);

// เชื่อมต่อ WiFi
  Serial.println();
  Serial.print("Wifi connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nWiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

// ตั้งค่า shunt ของ PZEM017 แต่ละตัว
  for (int i = 0; i < 4; i++)
  {
    setShunt(i + 1, shuntAddr[i]);
  }

  // เริ่มต้น DHT sensor
  dht.begin();

  // เชื่อมต่อ Server
  iotSetup();
}

void loop()
{
  iot.handle();

// อ่านค่าจากเซนเซอร์ ทุก 5 วินาที
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 5000)
  {
    previousMillis = currentMillis;
    updateTime += 5;   // อัพเดท soft timer(second) สำหรับการอัพเดทค่าไปยัง Server

    readDcSensor();  // อ่านค่าจาก PZEM017
    readAcSensor();  // อ่านค่าจาก PZEM004T

    // อ่านค่าจาก DHT22
    float t = dht.readTemperature();
    Serial.print(F("Temperature: "));
    Serial.print(t);
    Serial.println(F("°C "));

    // อัพเดทค่าไปยัง Server ทุก 1 ชั่วโมง(3600 วินาที)
    if (updateTime >= 3600)
    {
      updateTime = 0; // เครียรค่า soft timer

      // ส่งค่าไปยัง Serverโดยผ่านตัวแปรแบบ array
      float val[numVariables] = {DC_Voltage[0], DC_Current[0], DC_Power[0],
                                 DC_Voltage[1], DC_Current[1], DC_Power[1],
                                 DC_Voltage[2], DC_Current[2], DC_Power[2],
                                 DC_Voltage[3], DC_Current[3], DC_Power[3],
                                 AC_Voltage[0], AC_Current[0], AC_Power[0],
                                 AC_Voltage[1], AC_Current[1], AC_Power[1], t};

      iot.update(val);
    }
  }

} // Loop Ends

// ฟังก์ชันอ่านค่าจาก PZEM004T
void readAcSensor()
{
// สร้าง object ชื่อ iot จาก class PZEM004Tv30
  PZEM004Tv30 pzems[] = {PZEM004Tv30(Serial2, AC_PZEM_RX_PIN, AC_PZEM_TX_PIN, 0x05), PZEM004Tv30(Serial2, AC_PZEM_RX_PIN, AC_PZEM_TX_PIN, 0x06)}; // array of pzem 3 phase

// อ่านค่าจาก PZEM004T โดยวนลูป 2 ครั้ง
  for (int i = 0; i < 2; i++)
  {
    //------read data------
    AC_Voltage[i] = pzems[i].voltage();
    if (!isnan(AC_Voltage[i]))
    { // ถ้าอ่านค่าได้
      AC_Current[i] = pzems[i].current();
      AC_Power[i] = pzems[i].power();
      AC_Energy[i] = pzems[i].energy();
      AC_Frequency[i] = pzems[i].frequency();
      AC_PF[i] = pzems[i].pf();
    }
    else
    { // ถ้าอ่านค่าไม่ได้ให้ใส่ค่า NAN(not a number)
      AC_Current[i] = NAN;
      AC_Power[i] = NAN;
      AC_Energy[i] = NAN;
      AC_Frequency[i] = NAN;
      AC_PF[i] = NAN;
    }

    //แสดงค่าผ่าน Serial Monitor
    Serial.print(F("PZEM "));
    Serial.print(i);
    Serial.print(F(" - Address:"));
    Serial.println(pzems[i].getAddress(), HEX);
    Serial.println(F("==================="));
    if (!isnan(AC_Voltage[i]))
    {
      Serial.print(F("Voltage: "));
      Serial.print(AC_Voltage[i]);
      Serial.println("V");
      Serial.print(F("Current: "));
      Serial.print(AC_Current[i]);
      Serial.println(F("A"));
      Serial.print(F("Power: "));
      Serial.print(AC_Power[i]);
      Serial.println(F("W"));
      Serial.print(F("Energy: "));
      Serial.print(AC_Energy[i], 3);
      Serial.println(F("kWh"));
      Serial.print(F("Frequency: "));
      Serial.print(AC_Frequency[i], 1);
      Serial.println(F("Hz"));
      Serial.print(F("PF: "));
      Serial.println(AC_PF[i]);
    }
    else
    {
      Serial.println("No sensor detect");
    }
    Serial.println(F("-------------------"));
    Serial.println();
  }
}

// ฟังก์ชันอ่านค่าจาก PZEM017
void readDcSensor()
{
  // สร้าง object ชื่อ node จาก class ModbusMaster เพื่อใช้ในการอ่านค่าจาก PZEM017 4 ตัว
  ModbusMaster node1;
  ModbusMaster node2;
  ModbusMaster node3;
  ModbusMaster node4;

// อ่านค่าจาก PZEM017 ตัวที่1 โดยโปรโตคอล modbus RTU
  node1.begin(0x01, Serial1);
  uint8_t result;                               /* Declare variable "result" as 8 bits */
  result = node1.readInputRegisters(0x0000, 6); /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
  if (result == node1.ku8MBSuccess)             /* If there is a response */
  {
    uint32_t tempdouble = 0x00000000;                        /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
    DC_Voltage[0] = node1.getResponseBuffer(0x0000) / 100.0; /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
    // 0x0000 to 0x0008 are the register address of the measurement value
    DC_Current[0] = node1.getResponseBuffer(0x0001) / 100.0; /* get the 16bit value for the current value, divide it by 100 (as per manual) */

    tempdouble = (node1.getResponseBuffer(0x0003) << 16) + node1.getResponseBuffer(0x0002); /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Power[0] = tempdouble / 10.0;                                                        /* Divide the value by 10 to get actual power value (as per manual) */

    tempdouble = (node1.getResponseBuffer(0x0005) << 16) + node1.getResponseBuffer(0x0004); /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Energy[0] = tempdouble;
    DC_Energy[0] /= 1000; // to kWh

// แสดงค่าผ่าน Serial Monitor
    Serial.println(F("==================="));
    Serial.print(DC_Voltage[0], 1); // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[0], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[0], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[0], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  }
  else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
  {
    DC_Voltage[0] = NAN;
    DC_Current[0] = NAN;
    DC_Power[0] = NAN;
    DC_Energy[0] = NAN;

    Serial.println(F("==================="));
    Serial.println("Read error");
    Serial.println(F("-------------------"));
    Serial.println();
  }

// อ่านค่าจาก PZEM017 ตัวที่2 โดยโปรโตคอล modbus RTU
  node2.begin(0x02, Serial1);
  result = node2.readInputRegisters(0x0000, 6); /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
  if (result == node2.ku8MBSuccess)             /* If there is a response */
  {
    uint32_t tempdouble = 0x00000000;                        /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
    DC_Voltage[1] = node2.getResponseBuffer(0x0000) / 100.0; /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
    // 0x0000 to 0x0008 are the register address of the measurement value
    DC_Current[1] = node2.getResponseBuffer(0x0001) / 100.0; /* get the 16bit value for the current value, divide it by 100 (as per manual) */

    tempdouble = (node2.getResponseBuffer(0x0003) << 16) + node2.getResponseBuffer(0x0002); /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Power[1] = tempdouble / 10.0;                                                        /* Divide the value by 10 to get actual power value (as per manual) */

    tempdouble = (node2.getResponseBuffer(0x0005) << 16) + node2.getResponseBuffer(0x0004); /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Energy[1] = tempdouble;
    DC_Energy[1] /= 1000; // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[1], 1); // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[1], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[1], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[1], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  }
  else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
  {
    DC_Voltage[1] = NAN;
    DC_Current[1] = NAN;
    DC_Power[1] = NAN;
    DC_Energy[1] = NAN;

    Serial.println(F("==================="));
    Serial.println("Read error");
    Serial.println(F("-------------------"));
    Serial.println();
  }

// อ่านค่าจาก PZEM017 ตัวที่3 โดยโปรโตคอล modbus RTU
  node3.begin(0x03, Serial1);
  result = node3.readInputRegisters(0x0000, 6); /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
  if (result == node3.ku8MBSuccess)             /* If there is a response */
  {
    uint32_t tempdouble = 0x00000000;                        /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
    DC_Voltage[2] = node3.getResponseBuffer(0x0000) / 100.0; /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
    // 0x0000 to 0x0008 are the register address of the measurement value
    DC_Current[2] = node3.getResponseBuffer(0x0001) / 100.0; /* get the 16bit value for the current value, divide it by 100 (as per manual) */

    tempdouble = (node3.getResponseBuffer(0x0003) << 16) + node3.getResponseBuffer(0x0002); /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Power[2] = tempdouble / 10.0;                                                        /* Divide the value by 10 to get actual power value (as per manual) */

    tempdouble = (node3.getResponseBuffer(0x0005) << 16) + node3.getResponseBuffer(0x0004); /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Energy[2] = tempdouble;
    DC_Energy[2] /= 1000; // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[2], 1); // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[2], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[2], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[2], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  }
  else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
  {
    DC_Voltage[2] = NAN;
    DC_Current[2] = NAN;
    DC_Power[2] = NAN;
    DC_Energy[2] = NAN;

    Serial.println(F("==================="));
    Serial.println("Read error");
    Serial.println(F("-------------------"));
    Serial.println();
  }

// อ่านค่าจาก PZEM017 ตัวที่4 โดยโปรโตคอล modbus RTU
  node4.begin(0x04, Serial1);
  result = node4.readInputRegisters(0x0000, 6); /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
  if (result == node4.ku8MBSuccess)             /* If there is a response */
  {
    uint32_t tempdouble = 0x00000000;                        /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
    DC_Voltage[3] = node4.getResponseBuffer(0x0000) / 100.0; /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
    // 0x0000 to 0x0008 are the register address of the measurement value
    DC_Current[3] = node4.getResponseBuffer(0x0001) / 100.0; /* get the 16bit value for the current value, divide it by 100 (as per manual) */

    tempdouble = (node4.getResponseBuffer(0x0003) << 16) + node4.getResponseBuffer(0x0002); /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Power[3] = tempdouble / 10.0;                                                        /* Divide the value by 10 to get actual power value (as per manual) */

    tempdouble = (node4.getResponseBuffer(0x0005) << 16) + node4.getResponseBuffer(0x0004); /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
    DC_Energy[3] = tempdouble;
    DC_Energy[3] /= 1000; // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[3], 1); // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[3], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[3], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[3], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  }
  else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
  {
    DC_Voltage[3] = NAN;
    DC_Current[3] = NAN;
    DC_Power[3] = NAN;
    DC_Energy[3] = NAN;

    Serial.println(F("==================="));
    Serial.println("Read error");
    Serial.println(F("-------------------"));
    Serial.println();
  }
}

// ฟังก์ชันตั้งค่า shunt ของ PZEM017
void setShunt(uint8_t slaveAddr, uint16_t NewshuntAddr)
{
  static uint8_t SlaveParameter = 0x06;     /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0003; /* change shunt register address command code */

  uint16_t u16CRC = 0xFFFF;                 /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, slaveAddr); // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));

  Serial.println("Change shunt address");
  Serial1.write(slaveAddr); // these whole process code sequence refer to manual
  Serial1.write(SlaveParameter);
  Serial1.write(highByte(registerAddress));
  Serial1.write(lowByte(registerAddress));
  Serial1.write(highByte(NewshuntAddr));
  Serial1.write(lowByte(NewshuntAddr));
  Serial1.write(lowByte(u16CRC));
  Serial1.write(highByte(u16CRC));
  delay(10);
  delay(100);
  while (Serial1.available())
  {
    Serial.print(char(Serial1.read()), HEX); // Prints the response and display on Serial Monitor (Serial)
    Serial.print(" ");
  }
} // setShunt Ends