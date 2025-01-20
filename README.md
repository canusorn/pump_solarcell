# pump_solarcell

## อุปกรณ์
1. บอร์ด ESP32 พร้อมฐาน 1ตัว
2. pzem 004t v3  ac sensor 2ตัว
3. pzem017 50A dc sensor 2ตัว
4. pzem017 100A dc sensor 2ตัว
5. dht22 temp sensor 1ตัว

## Address เซนเซอร์แต่ละตัว
เซนเซอร์แต่ละตัว จะมี Address แยกแต่ละตัวให้แล้ว ทำให้ต่อขนานกันได้เลย

- PZEM017 1 - วัดไฟแผงเข้าชาร์จเจอร์
- PZEM017 2 - วัดไฟจากชาร์จเจอร์ เข้าแบตเตอร์รี่
- PZEM017 3 - วัดไฟจากแบตเตอร์รี่ เข้าอินเวอร์เตอร์ 220V
- PZEM017 4 - วัดไฟจากแบตเตอร์รี่ เข้าอินเวอร์เตอร์ปั้มน้ำ
- PZEM004T 1 - วัดไฟ ac เต้ารับ
- PZEM004T 2 - วัดไฟ ac ที่เข้าอินเวอร์เตอร์ปั้มน้ำ
## การต่อเซนเซอร์กับ ESP32

![PZEM017 dc sensor](pzem017.png)

![PZEM004T ac sensor](pzem004t.png)

![DHT22 Temp sensor](dht22.png)
