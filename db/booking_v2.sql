-- =============================================================================
-- CƠ SỞ DỮ LIỆU MOVIEGO V2 (FULL RE-INIT SCRIPT) - HỌC PHẦN IT4062 - NHÓM 9
-- Tác giả: Đàm Vĩnh Hưng (MSSV: 20235736) - Lớp Dữ liệu & Bảo mật
-- Thời gian: 19/11/2026
-- Mô tả: Khởi tạo toàn diện CSDL booking từ đầu, bao gồm bảng mở rộng v2,
--       chỉ mục tối ưu InnoDB, và seed dữ liệu chuẩn hóa 3 tài khoản gốc.
-- =============================================================================

DROP DATABASE IF EXISTS booking;
CREATE DATABASE booking CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE booking;

-- -----------------------------------------------------------------------------
-- 1. BẢNG USERS (Người dùng hệ thống)
-- role_id: 0 = Admin, 1 = Manager, 2 = Customer
-- Mật khẩu được băm bằng OpenSSL PKCS5_PBKDF2_HMAC (10.000 vòng) + Salt 32 hex chars
-- -----------------------------------------------------------------------------
CREATE TABLE users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    username VARCHAR(255) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    salt VARCHAR(64) DEFAULT '',
    role_id INT NOT NULL,
    balance DECIMAL(12,2) NOT NULL DEFAULT 500000.00,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 2. BẢNG CINEMAS (Rạp chiếu phim)
-- -----------------------------------------------------------------------------
CREATE TABLE cinemas (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    address VARCHAR(255) NOT NULL
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 3. BẢNG ROOMS (Phòng chiếu)
-- -----------------------------------------------------------------------------
CREATE TABLE rooms (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    cinema_id BIGINT UNSIGNED NOT NULL,
    total_seats INT NOT NULL,
    status ENUM('ACTIVE','INACTIVE') DEFAULT 'ACTIVE',
    FOREIGN KEY (cinema_id) REFERENCES cinemas(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 4. BẢNG CATEGORIES (Thể loại phim)
-- -----------------------------------------------------------------------------
CREATE TABLE categories (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 5. BẢNG FILMS (Phim chiếu)
-- -----------------------------------------------------------------------------
CREATE TABLE films (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    category_id BIGINT UNSIGNED NOT NULL,
    show_time BIGINT UNSIGNED NOT NULL,
    FOREIGN KEY (category_id) REFERENCES categories(id)
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 6. BẢNG SEATS (Ghế ngồi trong phòng chiếu)
-- -----------------------------------------------------------------------------
CREATE TABLE seats (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    room_id BIGINT UNSIGNED NOT NULL,
    seat_number VARCHAR(10) NOT NULL,
    row_name VARCHAR(5) NOT NULL,
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
    UNIQUE (room_id, seat_number)
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 7. BẢNG SHOWTIMES (Suất chiếu)
-- -----------------------------------------------------------------------------
CREATE TABLE showtimes (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    film_id BIGINT UNSIGNED NOT NULL,
    room_id BIGINT UNSIGNED NOT NULL,
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    -- Tuần 7: giá vé mỗi ghế. Server tự tính total_amount = price x số ghế (client không gửi giá).
    price DECIMAL(10,2) NOT NULL DEFAULT 80000.00,
    CHECK (price > 0),
    FOREIGN KEY (film_id) REFERENCES films(id),
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
    UNIQUE (room_id, start_time)
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 8. BẢNG BOOKINGS (Đơn đặt vé giữ chỗ 10 phút)
-- -----------------------------------------------------------------------------
CREATE TABLE bookings (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    showtime_id BIGINT UNSIGNED NOT NULL,
    total_amount DECIMAL(12,2) NOT NULL,
    status ENUM('PENDING', 'PAID', 'CANCELLED') NOT NULL DEFAULT 'PENDING',
    expires_at DATETIME NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (showtime_id) REFERENCES showtimes(id)
) ENGINE=InnoDB;

-- Chỉ mục tối ưu cho truy vấn đơn PENDING còn hạn (Điểm 1 Hybrid Plan)
CREATE INDEX idx_bookings_hold ON bookings(showtime_id, status, expires_at);

-- -----------------------------------------------------------------------------
-- 9. BẢNG BOOKING_SEATS (Chi tiết ghế trong đơn đặt, 1-10 ghế)
-- -----------------------------------------------------------------------------
CREATE TABLE booking_seats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    booking_id BIGINT UNSIGNED NOT NULL,
    seat_id BIGINT UNSIGNED NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    FOREIGN KEY (booking_id) REFERENCES bookings(id) ON DELETE CASCADE,
    FOREIGN KEY (seat_id) REFERENCES seats(id),
    UNIQUE (booking_id, seat_id)
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 10. BẢNG TICKETS (Vé chính thức sau khi thanh toán)
-- Lưu ý (Điểm 2 Hybrid Plan): BỎ cột status. Khi hủy vé, DELETE trực tiếp dòng vé
-- để nhả ràng buộc UNIQUE(showtime_id, seat_id) cho khách khác mua lại.
-- -----------------------------------------------------------------------------
CREATE TABLE tickets (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    showtime_id BIGINT UNSIGNED NOT NULL,
    seat_id BIGINT UNSIGNED NOT NULL,
    booking_id BIGINT UNSIGNED NULL,
    booked_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (showtime_id) REFERENCES showtimes(id),
    FOREIGN KEY (seat_id) REFERENCES seats(id),
    FOREIGN KEY (booking_id) REFERENCES bookings(id) ON DELETE SET NULL,
    UNIQUE (showtime_id, seat_id)
) ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- 11. BẢNG WALLET_TRANSACTIONS (Sổ cái giao dịch ví ảo)
-- -----------------------------------------------------------------------------
CREATE TABLE wallet_transactions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    booking_id BIGINT UNSIGNED NULL,
    amount DECIMAL(12,2) NOT NULL,
    type ENUM('PAYMENT', 'REFUND', 'TOPUP') NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
) ENGINE=InnoDB;


-- =============================================================================
-- DỮ LIỆU SEED KHỞI TẠO (SEED DATA)
-- =============================================================================

-- 1. USERS (3 tài khoản gốc: admin, manager, user - Mật khẩu mặc định: '123456')
-- Đã băm PBKDF2-HMAC-SHA256 (10.000 vòng) + Salt 32 hex chars riêng biệt
INSERT INTO users (id, name, username, password, salt, role_id, balance) VALUES
(1, 'System Administrator', 'admin', '17fac54b3784fb0baa385846b884b5176ff0c125ce26e61db707df10b2ca4949', 'f8d381e60b21ea6fd2fcaf37801ae925', 0, 500000.00),
(2, 'Cinema Manager', 'manager', '69383e5b52a1cbd011c38759fcc70b658a055cbd0ff6de564f02e645cd575300', '6816b4abbafd157e2d316eb86ac45244', 1, 500000.00),
(3, 'Standard Customer', 'user', '1515b6974a1dc9b61f96b058658a757536a71a00e837c396713fb3f3939462fd', '6379d3eb48401991f377d9c047c34d20', 2, 500000.00);

-- 2. CINEMAS
INSERT INTO cinemas (id, name, address) VALUES
(1, 'CGV Vincom', '191 Ba Trieu, Hai Ba Trung, Ha Noi'),
(2, 'Lotte Cinema', '54 Lieu Giai, Ba Dinh, Ha Noi'),
(3, 'Galaxy Cinema', '116 Nguyen Du, Hai Ba Trung, Ha Noi');

-- 3. ROOMS
INSERT INTO rooms (id, name, cinema_id, total_seats, status) VALUES
(1, 'Phong 1', 1, 100, 'ACTIVE'),
(2, 'Phong 2', 1, 80, 'ACTIVE'),
(3, 'Phong 3', 2, 120, 'ACTIVE'),
(4, 'Phong 4', 2, 90, 'ACTIVE'),
(5, 'Phong 5', 3, 110, 'ACTIVE');

-- 4. SEATS (Room 1: 100 ghế; Room 2: 80 ghế)
INSERT INTO seats (room_id, seat_number, row_name) VALUES
-- Room 1 (Row A -> J, mỗi hàng 10 ghế)
(1, 'A1', 'A'), (1, 'A2', 'A'), (1, 'A3', 'A'), (1, 'A4', 'A'), (1, 'A5', 'A'),
(1, 'A6', 'A'), (1, 'A7', 'A'), (1, 'A8', 'A'), (1, 'A9', 'A'), (1, 'A10', 'A'),
(1, 'B1', 'B'), (1, 'B2', 'B'), (1, 'B3', 'B'), (1, 'B4', 'B'), (1, 'B5', 'B'),
(1, 'B6', 'B'), (1, 'B7', 'B'), (1, 'B8', 'B'), (1, 'B9', 'B'), (1, 'B10', 'B'),
(1, 'C1', 'C'), (1, 'C2', 'C'), (1, 'C3', 'C'), (1, 'C4', 'C'), (1, 'C5', 'C'),
(1, 'C6', 'C'), (1, 'C7', 'C'), (1, 'C8', 'C'), (1, 'C9', 'C'), (1, 'C10', 'C'),
(1, 'D1', 'D'), (1, 'D2', 'D'), (1, 'D3', 'D'), (1, 'D4', 'D'), (1, 'D5', 'D'),
(1, 'D6', 'D'), (1, 'D7', 'D'), (1, 'D8', 'D'), (1, 'D9', 'D'), (1, 'D10', 'D'),
(1, 'E1', 'E'), (1, 'E2', 'E'), (1, 'E3', 'E'), (1, 'E4', 'E'), (1, 'E5', 'E'),
(1, 'E6', 'E'), (1, 'E7', 'E'), (1, 'E8', 'E'), (1, 'E9', 'E'), (1, 'E10', 'E'),
(1, 'F1', 'F'), (1, 'F2', 'F'), (1, 'F3', 'F'), (1, 'F4', 'F'), (1, 'F5', 'F'),
(1, 'F6', 'F'), (1, 'F7', 'F'), (1, 'F8', 'F'), (1, 'F9', 'F'), (1, 'F10', 'F'),
(1, 'G1', 'G'), (1, 'G2', 'G'), (1, 'G3', 'G'), (1, 'G4', 'G'), (1, 'G5', 'G'),
(1, 'G6', 'G'), (1, 'G7', 'G'), (1, 'G8', 'G'), (1, 'G9', 'G'), (1, 'G10', 'G'),
(1, 'H1', 'H'), (1, 'H2', 'H'), (1, 'H3', 'H'), (1, 'H4', 'H'), (1, 'H5', 'H'),
(1, 'H6', 'H'), (1, 'H7', 'H'), (1, 'H8', 'H'), (1, 'H9', 'H'), (1, 'H10', 'H'),
(1, 'I1', 'I'), (1, 'I2', 'I'), (1, 'I3', 'I'), (1, 'I4', 'I'), (1, 'I5', 'I'),
(1, 'I6', 'I'), (1, 'I7', 'I'), (1, 'I8', 'I'), (1, 'I9', 'I'), (1, 'I10', 'I'),
(1, 'J1', 'J'), (1, 'J2', 'J'), (1, 'J3', 'J'), (1, 'J4', 'J'), (1, 'J5', 'J'),
(1, 'J6', 'J'), (1, 'J7', 'J'), (1, 'J8', 'J'), (1, 'J9', 'J'), (1, 'J10', 'J');

-- Room 2 (Row A -> H, mỗi hàng 10 ghế)
INSERT INTO seats (room_id, seat_number, row_name) VALUES
(2, 'A1', 'A'), (2, 'A2', 'A'), (2, 'A3', 'A'), (2, 'A4', 'A'), (2, 'A5', 'A'),
(2, 'A6', 'A'), (2, 'A7', 'A'), (2, 'A8', 'A'), (2, 'A9', 'A'), (2, 'A10', 'A'),
(2, 'B1', 'B'), (2, 'B2', 'B'), (2, 'B3', 'B'), (2, 'B4', 'B'), (2, 'B5', 'B'),
(2, 'B6', 'B'), (2, 'B7', 'B'), (2, 'B8', 'B'), (2, 'B9', 'B'), (2, 'B10', 'B'),
(2, 'C1', 'C'), (2, 'C2', 'C'), (2, 'C3', 'C'), (2, 'C4', 'C'), (2, 'C5', 'C'),
(2, 'C6', 'C'), (2, 'C7', 'C'), (2, 'C8', 'C'), (2, 'C9', 'C'), (2, 'C10', 'C'),
(2, 'D1', 'D'), (2, 'D2', 'D'), (2, 'D3', 'D'), (2, 'D4', 'D'), (2, 'D5', 'D'),
(2, 'D6', 'D'), (2, 'D7', 'D'), (2, 'D8', 'D'), (2, 'D9', 'D'), (2, 'D10', 'D'),
(2, 'E1', 'E'), (2, 'E2', 'E'), (2, 'E3', 'E'), (2, 'E4', 'E'), (2, 'E5', 'E'),
(2, 'E6', 'E'), (2, 'E7', 'E'), (2, 'E8', 'E'), (2, 'E9', 'E'), (2, 'E10', 'E'),
(2, 'F1', 'F'), (2, 'F2', 'F'), (2, 'F3', 'F'), (2, 'F4', 'F'), (2, 'F5', 'F'),
(2, 'F6', 'F'), (2, 'F7', 'F'), (2, 'F8', 'F'), (2, 'F9', 'F'), (2, 'F10', 'F'),
(2, 'G1', 'G'), (2, 'G2', 'G'), (2, 'G3', 'G'), (2, 'G4', 'G'), (2, 'G5', 'G'),
(2, 'G6', 'G'), (2, 'G7', 'G'), (2, 'G8', 'G'), (2, 'G9', 'G'), (2, 'G10', 'G'),
(2, 'H1', 'H'), (2, 'H2', 'H'), (2, 'H3', 'H'), (2, 'H4', 'H'), (2, 'H5', 'H'),
(2, 'H6', 'H'), (2, 'H7', 'H'), (2, 'H8', 'H'), (2, 'H9', 'H'), (2, 'H10', 'H');

-- Room 3 (Row A -> L, mỗi hàng 10 ghế = 120 ghế)
INSERT INTO seats (room_id, seat_number, row_name) VALUES
(3, 'A1', 'A'), (3, 'A2', 'A'), (3, 'A3', 'A'), (3, 'A4', 'A'), (3, 'A5', 'A'), (3, 'A6', 'A'), (3, 'A7', 'A'), (3, 'A8', 'A'), (3, 'A9', 'A'), (3, 'A10', 'A'),
(3, 'B1', 'B'), (3, 'B2', 'B'), (3, 'B3', 'B'), (3, 'B4', 'B'), (3, 'B5', 'B'), (3, 'B6', 'B'), (3, 'B7', 'B'), (3, 'B8', 'B'), (3, 'B9', 'B'), (3, 'B10', 'B'),
(3, 'C1', 'C'), (3, 'C2', 'C'), (3, 'C3', 'C'), (3, 'C4', 'C'), (3, 'C5', 'C'), (3, 'C6', 'C'), (3, 'C7', 'C'), (3, 'C8', 'C'), (3, 'C9', 'C'), (3, 'C10', 'C'),
(3, 'D1', 'D'), (3, 'D2', 'D'), (3, 'D3', 'D'), (3, 'D4', 'D'), (3, 'D5', 'D'), (3, 'D6', 'D'), (3, 'D7', 'D'), (3, 'D8', 'D'), (3, 'D9', 'D'), (3, 'D10', 'D'),
(3, 'E1', 'E'), (3, 'E2', 'E'), (3, 'E3', 'E'), (3, 'E4', 'E'), (3, 'E5', 'E'), (3, 'E6', 'E'), (3, 'E7', 'E'), (3, 'E8', 'E'), (3, 'E9', 'E'), (3, 'E10', 'E'),
(3, 'F1', 'F'), (3, 'F2', 'F'), (3, 'F3', 'F'), (3, 'F4', 'F'), (3, 'F5', 'F'), (3, 'F6', 'F'), (3, 'F7', 'F'), (3, 'F8', 'F'), (3, 'F9', 'F'), (3, 'F10', 'F'),
(3, 'G1', 'G'), (3, 'G2', 'G'), (3, 'G3', 'G'), (3, 'G4', 'G'), (3, 'G5', 'G'), (3, 'G6', 'G'), (3, 'G7', 'G'), (3, 'G8', 'G'), (3, 'G9', 'G'), (3, 'G10', 'G'),
(3, 'H1', 'H'), (3, 'H2', 'H'), (3, 'H3', 'H'), (3, 'H4', 'H'), (3, 'H5', 'H'), (3, 'H6', 'H'), (3, 'H7', 'H'), (3, 'H8', 'H'), (3, 'H9', 'H'), (3, 'H10', 'H'),
(3, 'I1', 'I'), (3, 'I2', 'I'), (3, 'I3', 'I'), (3, 'I4', 'I'), (3, 'I5', 'I'), (3, 'I6', 'I'), (3, 'I7', 'I'), (3, 'I8', 'I'), (3, 'I9', 'I'), (3, 'I10', 'I'),
(3, 'J1', 'J'), (3, 'J2', 'J'), (3, 'J3', 'J'), (3, 'J4', 'J'), (3, 'J5', 'J'), (3, 'J6', 'J'), (3, 'J7', 'J'), (3, 'J8', 'J'), (3, 'J9', 'J'), (3, 'J10', 'J'),
(3, 'K1', 'K'), (3, 'K2', 'K'), (3, 'K3', 'K'), (3, 'K4', 'K'), (3, 'K5', 'K'), (3, 'K6', 'K'), (3, 'K7', 'K'), (3, 'K8', 'K'), (3, 'K9', 'K'), (3, 'K10', 'K'),
(3, 'L1', 'L'), (3, 'L2', 'L'), (3, 'L3', 'L'), (3, 'L4', 'L'), (3, 'L5', 'L'), (3, 'L6', 'L'), (3, 'L7', 'L'), (3, 'L8', 'L'), (3, 'L9', 'L'), (3, 'L10', 'L');

-- Room 4 (Row A -> I, mỗi hàng 10 ghế = 90 ghế)
INSERT INTO seats (room_id, seat_number, row_name) VALUES
(4, 'A1', 'A'), (4, 'A2', 'A'), (4, 'A3', 'A'), (4, 'A4', 'A'), (4, 'A5', 'A'), (4, 'A6', 'A'), (4, 'A7', 'A'), (4, 'A8', 'A'), (4, 'A9', 'A'), (4, 'A10', 'A'),
(4, 'B1', 'B'), (4, 'B2', 'B'), (4, 'B3', 'B'), (4, 'B4', 'B'), (4, 'B5', 'B'), (4, 'B6', 'B'), (4, 'B7', 'B'), (4, 'B8', 'B'), (4, 'B9', 'B'), (4, 'B10', 'B'),
(4, 'C1', 'C'), (4, 'C2', 'C'), (4, 'C3', 'C'), (4, 'C4', 'C'), (4, 'C5', 'C'), (4, 'C6', 'C'), (4, 'C7', 'C'), (4, 'C8', 'C'), (4, 'C9', 'C'), (4, 'C10', 'C'),
(4, 'D1', 'D'), (4, 'D2', 'D'), (4, 'D3', 'D'), (4, 'D4', 'D'), (4, 'D5', 'D'), (4, 'D6', 'D'), (4, 'D7', 'D'), (4, 'D8', 'D'), (4, 'D9', 'D'), (4, 'D10', 'D'),
(4, 'E1', 'E'), (4, 'E2', 'E'), (4, 'E3', 'E'), (4, 'E4', 'E'), (4, 'E5', 'E'), (4, 'E6', 'E'), (4, 'E7', 'E'), (4, 'E8', 'E'), (4, 'E9', 'E'), (4, 'E10', 'E'),
(4, 'F1', 'F'), (4, 'F2', 'F'), (4, 'F3', 'F'), (4, 'F4', 'F'), (4, 'F5', 'F'), (4, 'F6', 'F'), (4, 'F7', 'F'), (4, 'F8', 'F'), (4, 'F9', 'F'), (4, 'F10', 'F'),
(4, 'G1', 'G'), (4, 'G2', 'G'), (4, 'G3', 'G'), (4, 'G4', 'G'), (4, 'G5', 'G'), (4, 'G6', 'G'), (4, 'G7', 'G'), (4, 'G8', 'G'), (4, 'G9', 'G'), (4, 'G10', 'G'),
(4, 'H1', 'H'), (4, 'H2', 'H'), (4, 'H3', 'H'), (4, 'H4', 'H'), (4, 'H5', 'H'), (4, 'H6', 'H'), (4, 'H7', 'H'), (4, 'H8', 'H'), (4, 'H9', 'H'), (4, 'H10', 'H'),
(4, 'I1', 'I'), (4, 'I2', 'I'), (4, 'I3', 'I'), (4, 'I4', 'I'), (4, 'I5', 'I'), (4, 'I6', 'I'), (4, 'I7', 'I'), (4, 'I8', 'I'), (4, 'I9', 'I'), (4, 'I10', 'I');

-- Room 5 (Row A -> K, mỗi hàng 10 ghế = 110 ghế)
INSERT INTO seats (room_id, seat_number, row_name) VALUES
(5, 'A1', 'A'), (5, 'A2', 'A'), (5, 'A3', 'A'), (5, 'A4', 'A'), (5, 'A5', 'A'), (5, 'A6', 'A'), (5, 'A7', 'A'), (5, 'A8', 'A'), (5, 'A9', 'A'), (5, 'A10', 'A'),
(5, 'B1', 'B'), (5, 'B2', 'B'), (5, 'B3', 'B'), (5, 'B4', 'B'), (5, 'B5', 'B'), (5, 'B6', 'B'), (5, 'B7', 'B'), (5, 'B8', 'B'), (5, 'B9', 'B'), (5, 'B10', 'B'),
(5, 'C1', 'C'), (5, 'C2', 'C'), (5, 'C3', 'C'), (5, 'C4', 'C'), (5, 'C5', 'C'), (5, 'C6', 'C'), (5, 'C7', 'C'), (5, 'C8', 'C'), (5, 'C9', 'C'), (5, 'C10', 'C'),
(5, 'D1', 'D'), (5, 'D2', 'D'), (5, 'D3', 'D'), (5, 'D4', 'D'), (5, 'D5', 'D'), (5, 'D6', 'D'), (5, 'D7', 'D'), (5, 'D8', 'D'), (5, 'D9', 'D'), (5, 'D10', 'D'),
(5, 'E1', 'E'), (5, 'E2', 'E'), (5, 'E3', 'E'), (5, 'E4', 'E'), (5, 'E5', 'E'), (5, 'E6', 'E'), (5, 'E7', 'E'), (5, 'E8', 'E'), (5, 'E9', 'E'), (5, 'E10', 'E'),
(5, 'F1', 'F'), (5, 'F2', 'F'), (5, 'F3', 'F'), (5, 'F4', 'F'), (5, 'F5', 'F'), (5, 'F6', 'F'), (5, 'F7', 'F'), (5, 'F8', 'F'), (5, 'F9', 'F'), (5, 'F10', 'F'),
(5, 'G1', 'G'), (5, 'G2', 'G'), (5, 'G3', 'G'), (5, 'G4', 'G'), (5, 'G5', 'G'), (5, 'G6', 'G'), (5, 'G7', 'G'), (5, 'G8', 'G'), (5, 'G9', 'G'), (5, 'G10', 'G'),
(5, 'H1', 'H'), (5, 'H2', 'H'), (5, 'H3', 'H'), (5, 'H4', 'H'), (5, 'H5', 'H'), (5, 'H6', 'H'), (5, 'H7', 'H'), (5, 'H8', 'H'), (5, 'H9', 'H'), (5, 'H10', 'H'),
(5, 'I1', 'I'), (5, 'I2', 'I'), (5, 'I3', 'I'), (5, 'I4', 'I'), (5, 'I5', 'I'), (5, 'I6', 'I'), (5, 'I7', 'I'), (5, 'I8', 'I'), (5, 'I9', 'I'), (5, 'I10', 'I'),
(5, 'J1', 'J'), (5, 'J2', 'J'), (5, 'J3', 'J'), (5, 'J4', 'J'), (5, 'J5', 'J'), (5, 'J6', 'J'), (5, 'J7', 'J'), (5, 'J8', 'J'), (5, 'J9', 'J'), (5, 'J10', 'J'),
(5, 'K1', 'K'), (5, 'K2', 'K'), (5, 'K3', 'K'), (5, 'K4', 'K'), (5, 'K5', 'K'), (5, 'K6', 'K'), (5, 'K7', 'K'), (5, 'K8', 'K'), (5, 'K9', 'K'), (5, 'K10', 'K');

-- 5. CATEGORIES
INSERT INTO categories (id, name) VALUES
(1, 'Hanh dong'),
(2, 'Kinh di'),
(3, 'Hai huoc'),
(4, 'Tinh cam'),
(5, 'Khoa hoc vien tuong');

-- 6. FILMS
INSERT INTO films (id, title, category_id, show_time) VALUES
(1, 'Avengers: Endgame', 1, 180),
(2, 'The Conjuring 3', 2, 112),
(3, 'Mr. Bean Holiday', 3, 90),
(4, 'Titanic', 4, 195),
(5, 'Interstellar', 5, 169),
(6, 'Fast & Furious 9', 1, 143),
(7, 'It Chapter Two', 2, 169);

-- 7. SHOWTIMES
INSERT INTO showtimes (id, film_id, room_id, start_time, end_time) VALUES
(1, 1, 1, '2027-01-15 09:00:00', '2027-01-15 12:00:00'),
(2, 1, 1, '2027-01-15 14:00:00', '2027-01-15 17:00:00'),
(3, 1, 1, '2027-01-15 19:00:00', '2027-01-15 22:00:00'),
(4, 2, 2, '2027-01-15 11:30:00', '2027-01-15 13:22:00'),
(5, 2, 2, '2027-01-15 16:30:00', '2027-01-15 18:22:00'),
(6, 3, 2, '2027-01-15 09:00:00', '2027-01-15 10:30:00'),
(7, 3, 2, '2027-01-15 21:30:00', '2027-01-15 23:00:00'),
(8, 4, 3, '2027-01-15 11:30:00', '2027-01-15 14:45:00'),
(9, 4, 3, '2027-01-15 16:30:00', '2027-01-15 19:45:00'),
(10, 4, 3, '2027-01-15 21:30:00', '2027-01-16 00:45:00'),
(11, 5, 4, '2027-01-15 09:00:00', '2027-01-15 11:49:00'),
(12, 5, 4, '2027-01-15 19:00:00', '2027-01-15 21:49:00'),
(13, 6, 4, '2027-01-15 14:00:00', '2027-01-15 16:23:00'),
(14, 7, 5, '2027-01-15 09:00:00', '2027-01-15 11:49:00'),
(15, 7, 5, '2027-01-15 14:00:00', '2027-01-15 16:49:00'),
(16, 7, 5, '2027-01-15 19:00:00', '2027-01-15 21:49:00'),
(17, 1, 5, '2027-01-15 11:30:00', '2027-01-15 14:30:00'),
(18, 1, 5, '2027-01-15 21:30:00', '2027-01-16 00:30:00');

-- 8. TICKETS (Mẫu vé đã mua của user id=3)
INSERT INTO tickets (id, user_id, showtime_id, seat_id) VALUES
(1, 3, 1, 15),
(2, 3, 1, 16),
(3, 3, 2, 45),
(4, 3, 4, 122),
(5, 3, 6, 133);

