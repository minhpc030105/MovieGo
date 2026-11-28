-- =============================================================================
-- SCRIPT NÂNG CẤP CƠ SỞ DỮ LIỆU MOVIEGO V2 (MIGRATION SCRIPT CHO DB CÓ SẴN)
-- Học phần: IT4062 - Nhóm 9 (Đàm Vĩnh Hưng phụ trách)
-- Mô tả: Thêm các cột, bảng mới và cập nhật ghế/vé mà KHÔNG làm mất dữ liệu cũ
-- =============================================================================

USE booking;

-- 1. Bổ sung cột balance vào bảng users nếu chưa có
SET @dbname = DATABASE();
SET @tablename = "users";
SET @columnname = "balance";
SET @preparedStatement = (SELECT IF(
  (
    SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
    WHERE
      (table_name = @tablename)
      AND (table_schema = @dbname)
      AND (column_name = @columnname)
  ) > 0,
  "SELECT 1",
  "ALTER TABLE users ADD COLUMN balance DECIMAL(12,2) NOT NULL DEFAULT 500000.00;"
));
PREPARE alterIfNotExists FROM @preparedStatement;
EXECUTE alterIfNotExists;
DEALLOCATE PREPARE alterIfNotExists;

-- Cập nhật số dư 500.000 cho các user hiện tại nếu đang là 0 hoặc null
UPDATE users SET balance = 500000.00 WHERE balance IS NULL OR balance = 0.00;

-- 2. Bảng đơn đặt vé giữ chỗ 10 phút (bookings)
CREATE TABLE IF NOT EXISTS bookings (
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

-- Chỉ mục tối ưu cho truy vấn giữ chỗ PENDING còn hạn
SET @indexname = "idx_bookings_hold";
SET @preparedStatement = (SELECT IF(
  (
    SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS
    WHERE
      (table_name = "bookings")
      AND (table_schema = @dbname)
      AND (index_name = @indexname)
  ) > 0,
  "SELECT 1",
  "CREATE INDEX idx_bookings_hold ON bookings(showtime_id, status, expires_at);"
));
PREPARE createIndexIfNotExists FROM @preparedStatement;
EXECUTE createIndexIfNotExists;
DEALLOCATE PREPARE createIndexIfNotExists;

-- 3. Bảng chi tiết ghế đặt (booking_seats)
CREATE TABLE IF NOT EXISTS booking_seats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    booking_id BIGINT UNSIGNED NOT NULL,
    seat_id BIGINT UNSIGNED NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    FOREIGN KEY (booking_id) REFERENCES bookings(id) ON DELETE CASCADE,
    FOREIGN KEY (seat_id) REFERENCES seats(id),
    UNIQUE (booking_id, seat_id)
) ENGINE=InnoDB;

-- 4. Bảng sổ cái giao dịch ví ảo (wallet_transactions)
CREATE TABLE IF NOT EXISTS wallet_transactions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    booking_id BIGINT UNSIGNED NULL,
    amount DECIMAL(12,2) NOT NULL,
    type ENUM('PAYMENT', 'REFUND', 'TOPUP') NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
) ENGINE=InnoDB;

-- 5. Cập nhật bảng tickets: Thêm booking_id nếu chưa có
SET @columnname = "booking_id";
SET @tablename = "tickets";
SET @preparedStatement = (SELECT IF(
  (
    SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
    WHERE
      (table_name = @tablename)
      AND (table_schema = @dbname)
      AND (column_name = @columnname)
  ) > 0,
  "SELECT 1",
  "ALTER TABLE tickets ADD COLUMN booking_id BIGINT UNSIGNED NULL;"
));
PREPARE alterTickets FROM @preparedStatement;
EXECUTE alterTickets;
DEALLOCATE PREPARE alterTickets;

-- 6. Bổ sung ghế cho các phòng 3, 4, 5 (nếu chưa có)
INSERT IGNORE INTO seats (room_id, seat_number, row_name) VALUES
-- Room 3 (120 ghế: A1-L10)
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
(3, 'L1', 'L'), (3, 'L2', 'L'), (3, 'L3', 'L'), (3, 'L4', 'L'), (3, 'L5', 'L'), (3, 'L6', 'L'), (3, 'L7', 'L'), (3, 'L8', 'L'), (3, 'L9', 'L'), (3, 'L10', 'L'),
-- Room 4 (90 ghế: A1-I10)
(4, 'A1', 'A'), (4, 'A2', 'A'), (4, 'A3', 'A'), (4, 'A4', 'A'), (4, 'A5', 'A'), (4, 'A6', 'A'), (4, 'A7', 'A'), (4, 'A8', 'A'), (4, 'A9', 'A'), (4, 'A10', 'A'),
(4, 'B1', 'B'), (4, 'B2', 'B'), (4, 'B3', 'B'), (4, 'B4', 'B'), (4, 'B5', 'B'), (4, 'B6', 'B'), (4, 'B7', 'B'), (4, 'B8', 'B'), (4, 'B9', 'B'), (4, 'B10', 'B'),
(4, 'C1', 'C'), (4, 'C2', 'C'), (4, 'C3', 'C'), (4, 'C4', 'C'), (4, 'C5', 'C'), (4, 'C6', 'C'), (4, 'C7', 'C'), (4, 'C8', 'C'), (4, 'C9', 'C'), (4, 'C10', 'C'),
(4, 'D1', 'D'), (4, 'D2', 'D'), (4, 'D3', 'D'), (4, 'D4', 'D'), (4, 'D5', 'D'), (4, 'D6', 'D'), (4, 'D7', 'D'), (4, 'D8', 'D'), (4, 'D9', 'D'), (4, 'D10', 'D'),
(4, 'E1', 'E'), (4, 'E2', 'E'), (4, 'E3', 'E'), (4, 'E4', 'E'), (4, 'E5', 'E'), (4, 'E6', 'E'), (4, 'E7', 'E'), (4, 'E8', 'E'), (4, 'E9', 'E'), (4, 'E10', 'E'),
(4, 'F1', 'F'), (4, 'F2', 'F'), (4, 'F3', 'F'), (4, 'F4', 'F'), (4, 'F5', 'F'), (4, 'F6', 'F'), (4, 'F7', 'F'), (4, 'F8', 'F'), (4, 'F9', 'F'), (4, 'F10', 'F'),
(4, 'G1', 'G'), (4, 'G2', 'G'), (4, 'G3', 'G'), (4, 'G4', 'G'), (4, 'G5', 'G'), (4, 'G6', 'G'), (4, 'G7', 'G'), (4, 'G8', 'G'), (4, 'G9', 'G'), (4, 'G10', 'G'),
(4, 'H1', 'H'), (4, 'H2', 'H'), (4, 'H3', 'H'), (4, 'H4', 'H'), (4, 'H5', 'H'), (4, 'H6', 'H'), (4, 'H7', 'H'), (4, 'H8', 'H'), (4, 'H9', 'H'), (4, 'H10', 'H'),
(4, 'I1', 'I'), (4, 'I2', 'I'), (4, 'I3', 'I'), (4, 'I4', 'I'), (4, 'I5', 'I'), (4, 'I6', 'I'), (4, 'I7', 'I'), (4, 'I8', 'I'), (4, 'I9', 'I'), (4, 'I10', 'I'),
-- Room 5 (110 ghế: A1-K10)
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

-- 7. Sửa vé số 4 và 5 khớp phòng của suất chiếu (Suất 4 & 6 thuộc Room 2 -> seat 122 và 133)
UPDATE tickets SET seat_id = 122 WHERE id = 4 AND seat_id = 22;
UPDATE tickets SET seat_id = 133 WHERE id = 5 AND seat_id = 33;

-- 8. Bổ sung cột giá vé (price) vào bảng showtimes nếu chưa có (mặc định 75.000 VNĐ)
SET @columnname = "price";
SET @tablename = "showtimes";
SET @preparedStatement = (SELECT IF(
  (
    SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
    WHERE
      (table_name = @tablename)
      AND (table_schema = @dbname)
      AND (column_name = @columnname)
  ) > 0,
  "SELECT 1",
  "ALTER TABLE showtimes ADD COLUMN price DECIMAL(10,2) NOT NULL DEFAULT 75000.00 AFTER end_time;"
));
PREPARE alterShowtimesPrice FROM @preparedStatement;
EXECUTE alterShowtimesPrice;
DEALLOCATE PREPARE alterShowtimesPrice;
