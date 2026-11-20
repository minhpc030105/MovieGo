# MovieGo - Cinema Booking System

Hệ thống đặt vé xem phim theo mô hình Client-Server, dùng Socket TCP, pthread và MySQL.

Phạm vi hiện tại: server TCP đa luồng, giao thức văn bản riêng và chức năng tài khoản.

| Lệnh | Mô tả | Mã kết quả |
|---|---|---|
| `REGISTER` | Đăng ký tài khoản (vai trò User) | 1020 / 2021 |
| `LOGIN` | Đăng nhập theo 3 vai trò User, Manager, Admin | 1010, 1011, 1012 / 2011, 2012 |
| `LOGOUT` | Đăng xuất | 1030 |
| `CHANGE_PASSWORD` | Đổi mật khẩu | 1110 / 2110 |

Một tài khoản chỉ đăng nhập được ở một nơi. Khi client ngắt kết nối đột ngột, server tự đăng xuất tài khoản đó.

## Yêu cầu

Linux hoặc WSL2, GCC, MySQL Server 5.7+, `libmysqlclient-dev`.

## Cài đặt Database

### Cách 1: Khởi tạo toàn diện từ đầu (Khuyên dùng)
```bash
mysql -u root -p < db/booking_v2.sql
```
*(Lệnh này tự động tạo database `booking`, tạo toàn bộ 11 bảng chuẩn hóa và seed đầy đủ dữ liệu phòng, ghế, phim, suất chiếu cùng 3 tài khoản mẫu đã băm PBKDF2).*

### Cách 2: Khởi tạo cơ bản
```bash
mysql -u root -p < db/booking.sql
mysql -u root -p booking < db/data.txt
```

### Cách 3: Nâng cấp database đã có dữ liệu (Giữ nguyên dữ liệu cũ)
Nếu máy bạn đã có CSDL `booking` từ trước và muốn giữ nguyên dữ liệu:
```bash
mysql -u root -p booking < db/upgrade_v2.sql
```

Thông tin kết nối CSDL nằm trong `db/connect.h`.

## Biên dịch và chạy

```bash
make clean
make all
./server_out 8080
./client_out 127.0.0.1 8080
```

Tài khoản mẫu (Mật khẩu mặc định: `123456`):
- Admin: `admin` / `123456`
- Manager: `manager` / `123456`
- User: `user` / `123456` (Số dư ví ban đầu: 500.000đ)