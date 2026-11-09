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

## Tạo database

```bash
mysql -u root -p < db/booking.sql
mysql -u root -p booking < db/data.txt
```

Bỏ qua bước này nếu máy đã có database `booking`. Thông tin kết nối nằm trong `db/connect.h`.

## Biên dịch và chạy

```bash
make clean
make all
./server_out 8080
./client_out 127.0.0.1 8080
```

Tài khoản mẫu: `user1 / pass123` (User), `manager / manager123` (Manager), `longmh / 123456` (Admin).
