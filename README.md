# Embedded Smart Lock Project

Repository này chứa mã nguồn cho hệ thống khóa cửa thông minh dùng ESP32. Dự án gồm firmware điều khiển phần cứng và phần mobile app để quản lý/truy cập khóa.

## Thành phần chính

- `firmware-esp32/`: firmware ESP32 dùng PlatformIO/Arduino, xử lý RFID, keypad, LCD, buzzer, relay khóa và kết nối MQTT.
- `mobile-app/`: ứng dụng di động phục vụ điều khiển và quản lý hệ thống.

## Tính năng firmware

- Mở khóa bằng mật khẩu, thẻ RFID, nút bấm hoặc lệnh từ app.
- Gửi trạng thái khóa, pin và cảnh báo qua MQTT.
- Hỗ trợ thêm/xóa thẻ RFID từ xa.
- Tự khóa lại sau khi mở cửa.
- Có cơ chế khóa tạm thời khi nhập sai nhiều lần.

## Build firmware

```bash
cd firmware-esp32
make build
```

Upload lên ESP32:

```bash
make upload
```

Mở serial monitor:

```bash
make monitor
```

## Ghi chú

Firmware được tách thành nhiều module để dễ bảo trì, gồm xử lý khóa chính, theo dõi pin và helper MQTT.
