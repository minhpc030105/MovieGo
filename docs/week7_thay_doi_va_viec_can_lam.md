# Tuần 7 (Minh): thay đổi so với kế hoạch sprint và việc từng người cần điều chỉnh

Áp dụng lên bản `MovieGo.zip` (sau tuần 6: đã có pool, đã bỏ cache, dispatcher mượn kết nối theo từng request).

## 1. Đã làm gì (theo đúng việc tuần 7 của Minh)

| Việc trong kế hoạch | Kết quả |
|---|---|
| `SHOW_SEATS` 3 trạng thái | Mỗi ghế một dòng `SEAT\|id\|A-1\|AVAILABLE/HELD/BOOKED`. Đơn giữ ghế hết hạn tính là trống ngay (Điểm 6c) |
| `BOOK_TICKETS` | Giữ 1-10 ghế 10 phút. `SELECT ... FOR UPDATE` trên suất chiếu, kiểm tra ghế bán/giữ, một ghế trùng thì từ chối cả đơn (3032). `total_amount` do server tính từ `showtimes.price` |
| `PAY_BOOKING` | Một transaction: đổi PENDING thành PAID có điều kiện, trừ ví có điều kiện, phát hành vé, ghi `wallet_transactions`. Gọi lần hai trả 3033, không trừ tiền lần nữa |
| `CANCEL_BOOKING` | Hủy đơn PENDING của chính người gọi, ghế được nhả ngay |
| Sweeper | `sweeper_cancel_expired()` (SQL) và `sweeper_start()` (luồng nền, 10 giây/lần) |
| Bỏ cache (Lỗi 9) | Đã xong từ bản MovieGo.zip của bạn, giữ nguyên |

## 2. Thay đổi so với kế hoạch sprint

1. **Khóa theo suất chiếu trong tiến trình (mới, kế hoạch chưa có).** Bảng 256 khóa, chỉ số `showtime_id % 256`, chờ tối đa 30 giây (`booking_lock.c`). Mục đích: một suất chiếu đông người không làm cạn pool.
   Thứ tự bắt buộc: **khóa suất chiếu, rồi mượn kết nối, rồi transaction, rồi trả kết nối, rồi mở khóa.** Người chờ khóa không giữ kết nối nào, nên 100 client cùng đặt một suất chỉ dùng 1 kết nối và suất khác không bị `SERVER_BUSY`.
   `SELECT ... FOR UPDATE` và `UNIQUE(showtime_id, seat_id)` vẫn giữ nguyên làm lớp bảo vệ khi có nhiều tiến trình.
2. **Dispatcher có cờ `OWN_DB` (sửa `dispatch.c`).** Trước đây dispatcher mượn kết nối trước khi gọi handler, nên handler chờ khóa sẽ giữ kết nối (đúng lỗi starvation). Với `BOOK_TICKET`, `BOOK_TICKETS`, `PAY_BOOKING`, `CANCEL_BOOKING`, dispatcher chỉ mượn kết nối để kiểm tra vai trò, trả ngay, rồi handler tự mượn sau khi đã có khóa. Mọi lệnh khác chạy y như cũ. Với lệnh có `OWN_DB`, `session->db` là `NULL` khi vào handler.
3. **`SERVER_BUSY` có hai nguồn.** Hết pool sau 2 giây (như kế hoạch), hoặc không lấy được khóa suất chiếu sau 30 giây (mới). Chờ khóa là xếp hàng trong server, không phải lỗi. Thời gian chờ khóa là hằng số `SHOWTIME_LOCK_TIMEOUT_SEC` có thể đổi.
4. **`PAY_BOOKING` cũng lấy khóa suất chiếu** (kế hoạch không nói). Thiếu khóa này thì một lần thanh toán và một lần giữ ghế mới trên cùng suất có thể xen kẽ. `CANCEL_BOOKING` không cần khóa (một câu UPDATE có điều kiện, nhả ghế không thể gây đặt trùng).
5. **Thêm cột `showtimes.price`** (mặc định 80.000, phải lớn hơn 0). Kế hoạch yêu cầu server tính tiền từ giá suất chiếu nhưng schema chưa có cột giá. Có trong `booking_v2.sql` và `db/upgrade_week7.sql` (chạy lại nhiều lần vẫn an toàn). Suất thêm bằng `ADD_SHOWTIME` nhận giá mặc định.
6. **`SHOW_SEATS` đổi định dạng (đổi giao thức).** Trả mọi ghế kèm trạng thái thay vì chỉ ghế trống. Bản tuần 6 là `SEAT|id|A-1`, nay là `SEAT|id|A-1|STATE`. Mã `3020 NO_SEATS_AVAILABLE` không còn dùng.
7. **Giữ lệnh `BOOK_TICKET` cũ** (một ghế, vé phát ngay, không qua ví) để client tuần 6 vẫn chạy. Nó dùng cùng khóa và cùng quy tắc ghế trống nên không thể lấy ghế đang bị giữ. Xóa dòng đăng ký trong `dispatch.c` khi client đã chuyển sang `BOOK_TICKETS`.
8. **Mã kết quả mới:** `1210`, `1220`, `1230`, `3036 BOOKING_NOT_FOUND`, `3037 INVALID_SEAT_LIST`, `3038 BOOKING_NOT_PENDING`. Các mã `3031` đến `3035` đã có từ tuần 6 nay được phát thật.
9. **Lấy `booking_id` bằng `LAST_INSERT_ID()`** và kiểm tra ghế đã bán trước khi phát vé trong `PAY_BOOKING`, để `SEAT_TAKEN` không phụ thuộc việc đọc `mysql_errno` sau khi đóng statement.
10. **Sweeper mượn kết nối từ pool mỗi lượt** rồi trả, không giữ kết nối riêng. Kế hoạch chia Minh viết SQL, Cao Anh tạo luồng; ở đây `sweeper_start()` đã viết sẵn nên `server.c` chỉ thêm một dòng gọi.
11. **Chưa làm (không thuộc việc Minh tuần 7):** hoàn tiền khi hủy vé đã mua, `VIEW_PAYMENTS`, nạp tiền ví, `PING/PONG`, giới hạn 120 client, giao diện Qt.

## 3. Việc từng người cần điều chỉnh

### Cao Anh (mạng và điều phối, chủ `server.c`, `dispatch.c`)
- Review `dispatch.c`: cờ `OWN_DB`, hàm `check_access()` tách khỏi `route()`. Kiểm tra mọi đường thoát đều gọi `db_pool_release`.
- Review `server.c`: thêm `#include "headers/sweeper.h"` và một dòng `sweeper_start()` sau `initLogger()` (sau `db_pool_init`).
- Script tải 100 client (tuần 8): mỗi tài khoản chỉ đăng nhập được một nơi, nên cần 100 tài khoản. Kết quả đúng của mỗi request cùng một ghế là `1210` (một người) hoặc `3032` (còn lại). **`3031` là lỗi.** Tổng thời gian của suất chiếu đông sẽ dài hơn vì xếp hàng; suất khác không chậm.
- Timeout đọc của script và client phải lớn hơn thời gian xếp hàng (đặt từ 35 giây trở lên cho bài hot showtime).
- Nếu thử `poll()` + thread pool ở tuần 9: `showtime_lock` chặn luồng worker khi chờ. Cần tính đến khi so sánh, hoặc chuyển sang hàng đợi theo suất chiếu.
- Giới hạn 120 client: không liên quan đến khóa này, nhưng hãy cấu hình được và ghi số đo vào báo cáo.

### Hưng (dữ liệu và bảo mật, chủ `db/`, `db_pool.c`)
- Xác nhận tên cột `showtimes.price`, kiểu `DECIMAL(10,2)` và giá mặc định 80.000; chạy `upgrade_week7.sql` trên máy có dữ liệu cũ.
- `db_pool_release()` **không tự ROLLBACK**. Quy tắc hiện tại: handler phải COMMIT hoặc ROLLBACK trước khi trả. Mọi nhánh lỗi trong `booking.c` và `payment.c` đã làm vậy; hãy rà lại (đúng việc "rà soát transaction" của bạn) hoặc thêm ROLLBACK an toàn vào `db_pool_release`.
- `stmt_execute` trả `-1` cho mọi lỗi nên không phân biệt được trùng khóa và mất kết nối. Hiện chỉ `PAY_BOOKING` thử đọc `mysql_errno`. Nên có API trả mã lỗi MySQL.
- `handle_view_tickets` và `handle_ticket_detail` vẫn dùng `select_rows` (ghép chuỗi); chuyển sang prepared statement để hoàn tất Lỗi 6.
- Đặt `innodb_lock_wait_timeout` hợp lý. Pool dùng cờ `CLIENT_FOUND_ROWS` giống `connectDatabase`, nên số dòng bị ảnh hưởng nhất quán.
- `db/booking.sql` (cách khởi tạo cơ bản) không có các bảng đơn, vé, ví; chỉ `booking_v2.sql` hoặc `upgrade_v2.sql` + `upgrade_week7.sql` đủ cho tuần 7.

### Trang (client, Qt, tài liệu, kiểm thử)
- **`SHOW_SEATS`** nay trả cả ghế `HELD` và `BOOKED`. Console (`client/headers/function.c`, vòng lặp đọc `SEAT|`) hiện in tất cả ghế; phải lọc chỉ cho chọn `AVAILABLE`. Qt: tô màu theo trạng thái (trống, đang giữ, đã bán).
- **Đặt nhiều ghế:** gửi `BOOK_TICKETS` với `showtime_id` và danh sách `12,13,14`. Phản hồi `BOOKING|booking_id|PENDING|total|expires_at|seats`. Sau đó cho người dùng chọn `PAY_BOOKING` hoặc `CANCEL_BOOKING` bằng `booking_id` đó. Đồng hồ đếm ngược 10 phút tính từ lúc nhận phản hồi (không dùng múi giờ của `expires_at`).
- **Xử lý mã mới** trong `client_qt/responsecodes.h` và console: `1210`, `1220`, `1230`, `3031` (thử lại), `3032` (ghế vừa bị người khác lấy, tải lại sơ đồ ghế), `3033`, `3034` (đơn hết hạn), `3035` (không đủ số dư), `3036`, `3037`, `3038`.
- Số dư sau thanh toán nằm trong phản hồi `PAY_BOOKING` (trường cuối). Chưa có lệnh xem số dư riêng; làm cùng Minh ở tuần 8.
- `tests/test_week6.py`: đã sửa dòng kỳ vọng `SHOW_SEATS` thành `SEAT|1|A-1|AVAILABLE` và chạy thêm `upgrade_week7.sql`. `tests/regression_test.py` chưa kiểm tra định dạng ghế nên không đổi.
- `docs/protocol.md` đã cập nhật (các lệnh mới, mã mới, mục Open Issues); hãy đọc lại. Báo cáo 2 vẫn cần sửa: mục 2.2 (mỗi luồng một kết nối thay bằng pool mượn theo transaction), mục 2.3 (không còn END), và phần thanh toán (ví nội bộ, không phải VNPAY).
- `BOOK_TICKET` cũ vẫn dùng được trong lúc chuyển đổi.

## 4. Cách chạy và kiểm tra

```bash
git apply week7_booking.patch                       # hoặc chép các tệp trong week7_changed_files.zip
mysql -u root -p booking < db/upgrade_week7.sql     # chỉ cần nếu DB đã có từ trước (booking_v2.sql đã có cột price)
make clean && make all
make test_booking_lock && ./test_booking_lock_out   # không cần CSDL
./server_out 8080
python3 tests/test_week7_booking.py --port 8080 --showtime 1 --other-showtime 4
python3 tests/test_week6.py                         # cần MariaDB cục bộ
```

Đã kiểm tra: biên dịch từng tệp với `-Wall -Wextra` (dùng header MySQL giả để kiểm tra kiểu), `test_booking_lock` chạy sạch với ASan/UBSan, `test_week7_booking.py` qua `py_compile`, patch áp dụng sạch lên `MovieGo.zip`.
**Chưa chạy được** vì không có MySQL trong môi trường làm việc: các transaction thật, `test_week7_booking.py`, `test_week6.py`, và liên kết (link) cả server. Hãy chạy các bước trên máy nhóm trước khi merge.
`test_booking_lock` mô phỏng pool 10 kết nối: khóa trước thì 100 client cùng suất không ai thất bại và suất khác không bị chặn (hot showtime dùng tối đa 1 kết nối); đặt kết nối trước thì xảy ra timeout, đó chính là lỗi mà thứ tự mới loại bỏ.

## 5. Ghi chú cho pull request

- Thứ tự khóa (người review kiểm tra): khóa suất chiếu, mượn kết nối, transaction, trả kết nối, mở khóa. Không chờ khóa suất chiếu khi đang giữ kết nối, không lấy hai khóa cùng lúc.
- Khóa chỉ có tác dụng trong một tiến trình server; tính đúng đắn giữa nhiều tiến trình vẫn dựa vào `FOR UPDATE` và ràng buộc UNIQUE.
