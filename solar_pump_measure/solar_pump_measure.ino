// 18 > A
// 19 > B

// 16 > TX
// 17 > RX

// 26 > OUT

#include <WiFi.h>
#include <ModbusMaster.h>
#include <cynoiot.h>
#include <PZEM004Tv30.h>
#include "DHT.h"

const char ssid[] = "G6PD_2.4G";
const char pass[] = "570610193";
const char email[] = "timpiga.56789@gmail.com";

Cynoiot iot;

#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
#define DHTPIN 26
DHT dht(DHTPIN, DHTTYPE);

#define AC_PZEM_RX_PIN 16
#define AC_PZEM_TX_PIN 17
#define NUM_AC_PZEMS 1

#define DC_PZEM_RX_PIN 18
#define DC_PZEM_TX_PIN 19
#define NUM_DC_PZEMS 2

// ตั้งค่า shunt -->> 0x0000-100A, 0x0001-50A, 0x0002-200A, 0x0003-300A
uint16_t shuntAddr[] = { 0x0001, 0x0001, 0x0000, 0x0000 };

float DC_Voltage[4], DC_Current[4], DC_Power[4], DC_Energy[4];
float AC_Voltage[2], AC_Current[2], AC_Power[2], AC_Energy[2], AC_Frequency[2], AC_PF[2];

unsigned long previousMillis = 0;

uint8_t numVariables;
uint16_t updateTime = 3600;

void iotSetup() {
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

    "t"
  };
  iot.setkeyname(keyname, numVariables);
  // String keyname[2] = {"humid", "temp"};
  // iot.setkeyname(keyname, 2);

  Serial.println("ClinetID:" + String(iot.getClientId()));

  Serial.print("Connecting to server.");
  iot.connect(email);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N2, DC_PZEM_RX_PIN, DC_PZEM_TX_PIN);
  Serial2.begin(9600, SERIAL_8N2);

  Serial.println();
  Serial.print("Wifi connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nWiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // changeAddress(0xF8, 2);

  for (int i = 0; i < 4; i++) {
    setShunt(i + 1, shuntAddr[i]);
  }

  dht.begin();

  iotSetup();
}

void loop() {
  iot.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 5000) {
    previousMillis = currentMillis;
    updateTime += 5;

    readDcSensor();
    readAcSensor();
    float t = dht.readTemperature();
    Serial.print(F("Temperature: "));
    Serial.print(t);
    Serial.println(F("°C "));


    if (updateTime >= 3600) {
      updateTime = 0;

      float val[numVariables] = { DC_Voltage[0], DC_Current[0], DC_Power[0],
                                  DC_Voltage[1], DC_Current[1], DC_Power[1],
                                  DC_Voltage[2], DC_Current[2], DC_Power[2],
                                  DC_Voltage[3], DC_Current[3], DC_Power[3],
                                  AC_Voltage[0], AC_Current[0], AC_Power[0],
                                  AC_Voltage[1], AC_Current[1], AC_Power[1], t };

      iot.update(val);
    }



    //     float val[2] = {random(70, 80), random(20, 30)};
    // iot.update(val);
  }

}  // Loop Ends

void readAcSensor() {
  PZEM004Tv30 pzems[] = { PZEM004Tv30(Serial2, AC_PZEM_RX_PIN, AC_PZEM_TX_PIN, 0x05), PZEM004Tv30(Serial2, AC_PZEM_RX_PIN, AC_PZEM_TX_PIN, 0x06) };  // array of pzem 3 phase

  for (int i = 0; i < 2; i++) {
    //------read data------
    AC_Voltage[i] = pzems[i].voltage();
    if (!isnan(AC_Voltage[i])) {  // ถ้าอ่านค่าได้
      AC_Current[i] = pzems[i].current();
      AC_Power[i] = pzems[i].power();
      AC_Energy[i] = pzems[i].energy();
      AC_Frequency[i] = pzems[i].frequency();
      AC_PF[i] = pzems[i].pf();
    } else {  // ถ้าอ่านค่าไม่ได้ให้ใส่ค่า NAN(not a number)
      AC_Current[i] = NAN;
      AC_Power[i] = NAN;
      AC_Energy[i] = NAN;
      AC_Frequency[i] = NAN;
      AC_PF[i] = NAN;
    }

    //------Serial display------
    Serial.print(F("PZEM "));
    Serial.print(i);
    Serial.print(F(" - Address:"));
    Serial.println(pzems[i].getAddress(), HEX);
    Serial.println(F("==================="));
    if (!isnan(AC_Voltage[i])) {
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
    } else {
      Serial.println("No sensor detect");
    }
    Serial.println(F("-------------------"));
    Serial.println();
  }
}

void readDcSensor() {
  ModbusMaster node1;
  ModbusMaster node2;
  ModbusMaster node3;
  ModbusMaster node4;

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
    DC_Energy[0] /= 1000;  // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[0], 1);  // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[0], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[0], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[0], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  } else  // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
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
    DC_Energy[1] /= 1000;  // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[1], 1);  // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[1], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[1], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[1], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  } else  // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
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
    DC_Energy[2] /= 1000;  // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[2], 1);  // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[2], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[2], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[2], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  } else  // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
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
    DC_Energy[3] /= 1000;  // to kWh

    Serial.println(F("==================="));
    Serial.print(DC_Voltage[3], 1);  // Print Voltage value on Serial Monitor with 1 decimal*/
    Serial.print("V   ");
    Serial.print(DC_Current[3], 3);
    Serial.print("A   ");
    Serial.print(DC_Power[3], 1);
    Serial.print("W  ");
    Serial.print(DC_Energy[3], 0);
    Serial.println("kWh  ");
    Serial.println(F("-------------------"));
    Serial.println();
  } else  // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
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
void setShunt(uint8_t slaveAddr, uint16_t NewshuntAddr) {
  static uint8_t SlaveParameter = 0x06;     /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0003; /* change shunt register address command code */

  uint16_t u16CRC = 0xFFFF;                  /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, slaveAddr);  // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));

  Serial.println("Change shunt address");
  Serial1.write(slaveAddr);  // these whole process code sequence refer to manual
  Serial1.write(SlaveParameter);
  Serial1.write(highByte(registerAddress));
  Serial1.write(lowByte(registerAddress));
  Serial1.write(highByte(NewshuntAddr));
  Serial1.write(lowByte(NewshuntAddr));
  Serial1.write(lowByte(u16CRC));
  Serial1.write(highByte(u16CRC));
  delay(10);
  delay(100);
  while (Serial1.available()) {
    Serial.print(char(Serial1.read()), HEX);  // Prints the response and display on Serial Monitor (Serial)
    Serial.print(" ");
  }
}  // setShunt Ends

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr)  // Change the slave address of a node
{

  /* 1- PZEM-017 DC Energy Meter */

  static uint8_t SlaveParameter = 0x06;         /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0002;     /* Modbus RTU device address command code */
  uint16_t u16CRC = 0xFFFF;                     /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, OldslaveAddr);  // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
  Serial1.write(OldslaveAddr); /* these whole process code sequence refer to manual*/
  Serial1.write(SlaveParameter);
  Serial1.write(highByte(registerAddress));
  Serial1.write(lowByte(registerAddress));
  Serial1.write(highByte(NewslaveAddr));
  Serial1.write(lowByte(NewslaveAddr));
  Serial1.write(lowByte(u16CRC));
  Serial1.write(highByte(u16CRC));
  delay(100);
}