#include <ModbusMaster.h> // ติดตั้งจาก Library Manager ค้นหา ModbusMaster

// กำหนดขา RX TX ของ PZEM017
#define DC_PZEM_RX_PIN 18
#define DC_PZEM_TX_PIN 19

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N2, DC_PZEM_RX_PIN, DC_PZEM_TX_PIN);

    // เปลี่ยน slave address เป็น 0x02
    changeAddress(0xF8, 0x02);
}

void loop()
{
}

// ฟังก์ชันเปลี่ยน slave address ของ PZEM017
void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr) // Change the slave address of a node
{

    /* 1- PZEM-017 DC Energy Meter */

    static uint8_t SlaveParameter = 0x06;        /* Write command code to PZEM */
    static uint16_t registerAddress = 0x0002;    /* Modbus RTU device address command code */
    uint16_t u16CRC = 0xFFFF;                    /* declare CRC check 16 bits*/
    u16CRC = crc16_update(u16CRC, OldslaveAddr); // Calculate the crc16 over the 6bytes to be send
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
