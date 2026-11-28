-- =============================================================================
-- TUẦN 7: giá vé theo suất chiếu (server tự tính total_amount = price x số ghế)
-- Chạy sau upgrade_v2.sql. Chạy lại nhiều lần vẫn an toàn (bỏ qua nếu cột đã có).
-- Dùng:  mysql -u root -p booking < db/upgrade_week7.sql
-- Cần Hưng xác nhận tên cột `price` và giá mặc định 80.000.
-- =============================================================================
USE booking;

SET @has_price = (SELECT COUNT(*) FROM information_schema.COLUMNS
                  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'showtimes' AND COLUMN_NAME = 'price');
SET @ddl = IF(@has_price = 0,
    'ALTER TABLE showtimes ADD COLUMN price DECIMAL(10,2) NOT NULL DEFAULT 80000.00, ADD CONSTRAINT chk_showtimes_price CHECK (price > 0)',
    'SELECT 1');
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
