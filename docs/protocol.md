# MovieGo Network Protocol

## 1. Tổng quan

MovieGo dùng TCP để trao đổi lệnh tài khoản, danh mục phim, suất chiếu, vé và quản trị. Client gửi trước, không có lời chào khi kết nối. Server xử lý lần lượt trên từng socket và trả một response cho mỗi request có khung hợp lệ, kể cả khi lệnh bị từ chối. Có thể gửi nhiều khung liền nhau và đọc response theo cùng thứ tự.

Tên lệnh, dấu phân cách và mã kết quả dùng ASCII. Qt mã hóa dữ liệu nhập bằng UTF-8. Client console chuyển byte từ đầu vào, còn lớp khung và parser server không xác thực UTF-8 hay khai báo charset trên dây.

## 2. Message Framing

Cả hai chiều dùng `[LENGTH][PAYLOAD]`. LENGTH là số nguyên không dấu 32 bit, **4 byte big-endian**, đếm từ 1 đến 65.536 byte payload. Nếu có CRLF, hai byte đó nằm trong LENGTH. Byte NUL kết thúc chuỗi C không được gửi; NUL trong payload bị từ chối.

Đọc đúng 4 byte header, kiểm tra LENGTH, rồi lặp đọc cho đến khi đủ LENGTH byte, kể cả khi TCP chia nhỏ hoặc gộp khung. Gửi bằng vòng lặp gửi đủ byte. Payload `SHOW_FILMS\r\n` có header hex `00 00 00 0c`. Khung sai hoặc EOF giữa khung làm server đóng socket và dọn phiên. Server thiết lập timeout nhận 30 phút trên socket.

## 3. Request Format

`COMMAND[\r\narg1[\r\narg2...]][\r\n]`. Tên lệnh phân biệt hoa thường, tham số theo thứ tự và mỗi trường chiếm một dòng. Một CRLF cuối là tùy chọn. Không có dòng trống hoặc CR/LF đơn. Parser kiểm tra đúng số trường, tối đa 255 byte mỗi trường. Các tham số ID và số phút dùng số thập phân dương tới `INT_MAX`; `role_id` dùng một ký tự `0`, `1` hoặc `2`. Khoảng trắng trong trường văn bản được giữ nguyên.

## 4. Response Format

`CODE[\r\nline1[\r\nline2...]]`. CODE luôn là 4 chữ số ASCII. Mã thành công có thể đứng một mình hoặc theo sau bởi các dòng dữ liệu. Dispatcher bỏ body khi mã từ 2000 trở lên. Ví dụ payload thành công `1073\r\n1. First` và payload lỗi `3008`. Body vượt giới hạn trả `3030`; kết nối vẫn dùng tiếp được. Khi tạo dòng phản hồi, server thay CR/LF bên trong giá trị dữ liệu bằng khoảng trắng.

Console và Qt dùng `receive_response` để đọc đủ khung, kiểm tra dòng mã rồi duyệt body bằng `next_response_line`. `recvResult` đọc cả khung và chỉ trả mã. Hai hàm đọc trả `-1` khi lỗi I/O, khung hoặc dòng mã.

## 5. Authentication và Session

- Mỗi kết nối khởi đầu với `is_auth=false`, `user_id=0`, `role_id=-1` và username rỗng. LOGIN đối chiếu username, password và vai trò trong CSDL, rồi lưu `user_id`, `username`, `role_id`, bật `is_auth`. Cùng `user_id` chỉ có thể hoạt động trên một kết nối.
- Vai trò trong CSDL là Admin (0), Manager (1), User (2). LOGIN trả 1012, 1011 hoặc 1010 tương ứng. LOGIN thành công chỉ trả mã, UI dùng mã để chọn vai trò.
- LOGOUT xóa phiên và giữ socket mở. EXIT trả 1030 rồi đóng socket. EOF hoặc lỗi đọc/gửi cũng dọn phiên.
- Server tra lại vai trò theo `Session.user_id` trước khi chạy lệnh đã biết trên phiên đăng nhập, trừ LOGOUT và EXIT. Nếu tài khoản mất hoặc vai trò không hợp lệ, server xóa phiên và trả 3028; lỗi truy vấn trả 3022.
- CHANGE_PASSWORD, BOOK_TICKET, VIEW_TICKETS và VIEW_TICKET_DETAIL dùng `Session.user_id` làm danh tính người gọi. ID trong DELETE_USER và CHANGE_USER_ROLE là tài khoản đích.

## 6. Command Reference

Cột lỗi ở từng lệnh liệt kê các mã nghiệp vụ. Lỗi chung gồm `3018` (lệnh/cú pháp sai), `3028` (chưa đăng nhập hoặc mất phiên), `3029` (vai trò/trạng thái không cho phép), `3022` (lỗi tra lại vai trò) và `3030` (body danh sách/chi tiết quá lớn). Quyền được xét trước khi handler kiểm tra số và kiểu tham số. Các ví dụ là payload; ký hiệu `\r\n` biểu thị hai byte phân cách.

Console có đường gửi cho cả 26 lệnh. Qt dùng cùng thư viện khung, gửi các lệnh tài khoản, duyệt/tìm/đặt/xem vé và quản lý từ các dialog tương ứng.

Dòng chi tiết phim dùng chung mẫu `ID: id | Title: title | Category: category | Show_time: minutes minutes`, trong đó `minutes` đầu là giá trị số.

### LOGIN

- **Request** `LOGIN` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** username, password (văn bản).
- **Required authentication** Khách.
- **Required role** —.
- **Success response** 1010 User / 1011 Manager / 1012 Admin, chỉ mã.
- **Error response** 2011 không đăng nhập được, 2012 tài khoản hoặc socket đã đăng nhập, 3022 lỗi truy vấn.
- **Example** `LOGIN\r\nuser1\r\npass123`.

### REGISTER

- **Request** `REGISTER` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** name, username, password (văn bản).
- **Required authentication** Khách.
- **Required role** —.
- **Success response** 1020, tạo tài khoản User.
- **Error response** 2021 trùng username hoặc ghi thất bại.
- **Example** `REGISTER\r\nTrang\r\ntrangdemo\r\ndemo123`.

### LOGOUT

- **Request** `LOGOUT` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** Mọi vai trò.
- **Success response** 1030, xóa phiên.
- **Error response** Chỉ mã lỗi chung.
- **Example** `LOGOUT`.

### EXIT

- **Request** `EXIT` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Tùy chọn.
- **Required role** Mọi vai trò hoặc khách.
- **Success response** 1030, sau đó EOF.
- **Error response** Chỉ mã lỗi chung.
- **Example** `EXIT`.

### CHANGE_PASSWORD

- **Request** `CHANGE_PASSWORD` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** old_password, new_password (văn bản).
- **Required authentication** Có.
- **Required role** Mọi vai trò.
- **Success response** 1110, chỉ mã.
- **Error response** 2110 mật khẩu cũ sai hoặc cập nhật thất bại.
- **Example** `CHANGE_PASSWORD\r\nold123\r\nnew123`.

### SHOW_CATEGORIES

- **Request** `SHOW_CATEGORIES` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1065, mỗi dòng `id. name`.
- **Error response** 3001 lỗi đọc, 3002 trống.
- **Example** `SHOW_CATEGORIES`.

### BROWSE_CATEGORY

- **Request** `BROWSE_CATEGORY` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** category_id.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1070, các dòng chi tiết phim.
- **Error response** 2071 trống hoặc lỗi đọc.
- **Example** `BROWSE_CATEGORY\r\n1`.

### SHOW_CINEMAS

- **Request** `SHOW_CINEMAS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1071, mỗi dòng `id. name`.
- **Error response** 3003 lỗi đọc, 3004 trống.
- **Example** `SHOW_CINEMAS`.

### BROWSE_CINEMA

- **Request** `BROWSE_CINEMA` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** cinema_id.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1071, các dòng chi tiết phim có suất tại rạp.
- **Error response** 2071 trống hoặc lỗi đọc.
- **Example** `BROWSE_CINEMA\r\n1`.

### SHOW_PREMIERED_TIME

- **Request** `SHOW_PREMIERED_TIME` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1072, mỗi dòng `HH:MM - HH:MM`.
- **Error response** 3005 lỗi đọc, 3006 trống.
- **Example** `SHOW_PREMIERED_TIME`.

### BROWSE_PREMIERED_TIME

- **Request** `BROWSE_PREMIERED_TIME` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** start_time (văn bản đối chiếu với HH:MM).
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1072, các dòng chi tiết phim.
- **Error response** 2071 trống hoặc lỗi đọc.
- **Example** `BROWSE_PREMIERED_TIME\r\n10:00`.

### SHOW_FILMS

- **Request** `SHOW_FILMS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1073, mỗi dòng `id. title`.
- **Error response** 3007 lỗi đọc, 3008 trống.
- **Example** `SHOW_FILMS`.

### SHOW_CINEMA_BY_FILM

- **Request** `SHOW_CINEMA_BY_FILM` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** film_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1071, mỗi dòng `id. name`.
- **Error response** 3003 lỗi đọc, 3004 trống.
- **Example** `SHOW_CINEMA_BY_FILM\r\n1`.

### SHOW_TIME_BY_FILM_CINEMA

- **Request** `SHOW_TIME_BY_FILM_CINEMA` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** film_id, cinema_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1072, mỗi dòng `showtime_id. start_datetime - end_datetime`.
- **Error response** 3005 lỗi đọc, 3006 trống.
- **Example** `SHOW_TIME_BY_FILM_CINEMA\r\n1\r\n1`.

### SHOW_SEATS

- **Request** `SHOW_SEATS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** showtime_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1080, một dòng cho MỖI ghế của phòng: `SEAT|seat_id|row_name-seat_number|STATE`. `STATE` là `AVAILABLE` (trống), `HELD` (đang được giữ bởi đơn PENDING chưa hết hạn) hoặc `BOOKED` (đã có vé). Đơn giữ ghế quá `expires_at` được tính là `AVAILABLE` ngay, không chờ sweeper. Client chỉ cho chọn ghế `AVAILABLE`.
- **Error response** 3019 không có suất chiếu, 3010 phòng không có ghế, 3009 lỗi đọc CSDL.
- **Example** `SHOW_SEATS\r\n1` -> `1080\r\nSEAT|1|A-1|AVAILABLE\r\nSEAT|2|A-2|HELD\r\nSEAT|3|A-3|BOOKED`.

### BOOK_TICKETS

- **Request** `BOOK_TICKETS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** showtime_id, danh sách seat_id cách nhau bằng dấu phẩy (1 đến 10 ghế, chỉ chữ số và dấu phẩy, không trùng, không khoảng trắng).
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1210 + một dòng `BOOKING|booking_id|PENDING|total_amount|expires_at|seat_ids`. Giữ ghế 10 phút. `total_amount` do server tính từ `showtimes.price`, client không gửi giá.
- **Error response** 3037 danh sách ghế sai định dạng hoặc ghế không thuộc phòng của suất, 3019 không có suất, 3032 có ít nhất một ghế đã bán hoặc đang bị giữ (từ chối CẢ đơn, không giữ ghế nào), 3031 server bận, 3022 lỗi CSDL.
- **Example** `BOOK_TICKETS\r\n1\r\n12,13,14`.
- **Ghi chú** Các yêu cầu cùng suất chiếu được xếp hàng trong server (khóa theo suất chiếu, chờ tối đa 30 giây) và KHÔNG giữ kết nối CSDL trong lúc chờ; suất chiếu khác không bị ảnh hưởng.

### PAY_BOOKING

- **Request** `PAY_BOOKING` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** booking_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1220 + một dòng `BOOKING|booking_id|PAID|total_amount|balance_after`. Một giao dịch: đổi PENDING thành PAID (có điều kiện), trừ ví, phát hành vé, ghi sổ cái `wallet_transactions`.
- **Error response** 3036 không có đơn hoặc không phải của người gọi, 3033 đơn đã thanh toán (không trừ tiền lần hai), 3034 đơn hết hạn hoặc đã hủy, 3035 không đủ số dư, 3032 ghế đã bán trong lúc chờ, 3031 server bận, 3022 lỗi CSDL.
- **Example** `PAY_BOOKING\r\n5`.

### CANCEL_BOOKING

- **Request** `CANCEL_BOOKING` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** booking_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1230, đơn PENDING chuyển CANCELLED, ghế được nhả ngay.
- **Error response** 3036 không có đơn hoặc không phải của người gọi, 3038 đơn đã thanh toán hoặc đã hủy (hoàn tiền vé đã mua là việc của tuần 8), 3031 server bận, 3022 lỗi CSDL.
- **Example** `CANCEL_BOOKING\r\n5`.

### BOOK_TICKET (cũ, giữ tạm)

- **Request** `BOOK_TICKET` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** showtime_id, seat_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1200, ghi ngay một vé (không qua ví). Dùng cùng khóa suất chiếu và cùng quy tắc ghế trống với BOOK_TICKETS nên không thể lấy ghế đang bị giữ.
- **Error response** 2200 suất/ghế không tồn tại, ghế đã bán hoặc đang bị giữ, ghi thất bại; 3031 server bận.
- **Example** `BOOK_TICKET\r\n1\r\n15`.
- **Ghi chú** Chỉ giữ để client tuần 6 chạy được. Xóa dòng đăng ký trong `dispatch.c` khi client đã chuyển sang BOOK_TICKETS.

### SEARCH_FILM_BY_TITLE

- **Request** `SEARCH_FILM_BY_TITLE` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** title (văn bản tìm bằng SQL LIKE).
- **Required authentication** Có.
- **Required role** User / Manager.
- **Success response** 1073, các dòng chi tiết phim.
- **Error response** 2072 trống hoặc lỗi đọc.
- **Example** `SEARCH_FILM_BY_TITLE\r\nAvengers`.

### ADD_FILM

- **Request** `ADD_FILM` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** title (văn bản), category_id, duration_minutes.
- **Required authentication** Có.
- **Required role** Manager.
- **Success response** 1040, chỉ mã.
- **Error response** 2041 trùng tên, danh mục không tồn tại hoặc ghi thất bại.
- **Example** `ADD_FILM\r\nNew Film\r\n1\r\n90`.

### VIEW_TICKETS

- **Request** `VIEW_TICKETS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1080, bảng có cột ID, Film, Cinema, Room, Seat, Start Time, Booked At, rồi dòng `You has total: N ticket(s)`.
- **Error response** 3013 chưa có vé, 3014 lỗi đọc.
- **Example** `VIEW_TICKETS`.

### VIEW_TICKET_DETAIL

- **Request** `VIEW_TICKET_DETAIL` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** ticket_id.
- **Required authentication** Có.
- **Required role** User.
- **Success response** 1080, dòng `=== TICKET DETAIL ===`, rồi Customer, Film, Cinema/Address, Room/Capacity/Seat, Showtime, Booked at.
- **Error response** 3015 vé không tồn tại hoặc không thuộc phiên, 2081 lỗi đọc.
- **Example** `VIEW_TICKET_DETAIL\r\n1`.

### SHOW_ROOMS_BY_CINEMA

- **Request** `SHOW_ROOMS_BY_CINEMA` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** cinema_id.
- **Required authentication** Có.
- **Required role** Manager.
- **Success response** 1080, mỗi phòng ACTIVE có dòng `ID: id | name | total_seats seats`.
- **Error response** 3016 rạp không có, 3022 lỗi kiểm tra rạp, 3017 lỗi đọc phòng, 3012 không có phòng ACTIVE.
- **Example** `SHOW_ROOMS_BY_CINEMA\r\n1`.

### SHOW_SHOWTIMES_BY_ROOM

- **Request** `SHOW_SHOWTIMES_BY_ROOM` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** room_id.
- **Required authentication** Có.
- **Required role** Manager.
- **Success response** 1091, mỗi dòng `ID: id | Title: title | start_datetime - end_datetime (N minutes)`, từ ngày hiện tại theo CSDL.
- **Error response** 2092 trống hoặc lỗi đọc.
- **Example** `SHOW_SHOWTIMES_BY_ROOM\r\n1`.

### ADD_SHOWTIME

- **Request** `ADD_SHOWTIME` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** film_id, cinema_id, room_id, start_datetime (văn bản, ví dụ YYYY-MM-DD HH:MM:SS).
- **Required authentication** Có.
- **Required role** Manager.
- **Success response** 1092, chỉ mã, thời lượng lấy từ phim và cách suất khác ít nhất 15 phút.
- **Error response** 3021 phim không hợp lệ, 3022 lỗi truy vấn, 3023 rạp sai, 3024 phòng không ACTIVE/không tồn tại, 3025 phòng sai rạp, 3026 thời điểm không hợp lệ, 3027 xung đột, 2090 lỗi ghi.
- **Example** `ADD_SHOWTIME\r\n1\r\n1\r\n1\r\n2030-01-01 13:00:00`.

### SHOW_ALL_USERS

- **Request** `SHOW_ALL_USERS` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** —.
- **Required authentication** Có.
- **Required role** Admin.
- **Success response** 1300, mỗi dòng `id|name|username|role_name`.
- **Error response** 2300 lỗi đọc.
- **Example** `SHOW_ALL_USERS`.

### DELETE_USER

- **Request** `DELETE_USER` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** user_id đích.
- **Required authentication** Có.
- **Required role** Admin.
- **Success response** 1301, chỉ mã.
- **Error response** 2301 xóa thất bại hoặc không có bản ghi.
- **Example** `DELETE_USER\r\n2`.

### CHANGE_USER_ROLE

- **Request** `CHANGE_USER_ROLE` theo sau bởi các tham số bên dưới theo thứ tự.
- **Parameters** user_id đích, role_id.
- **Required authentication** Có.
- **Required role** Admin.
- **Success response** 1302, chỉ mã.
- **Error response** 2302 cập nhật thất bại hoặc không có bản ghi.
- **Example** `CHANGE_USER_ROLE\r\n2\r\n1`.

## 7. Result Codes

Tên mã từ `lib/resultCodes.h`. Cột **Used by** dựa trên dispatcher và handlers được build hiện tại.

| Code | Meaning | Used by |
| --- | --- | --- |
| 1010 | `LOGIN_SUCCESS_USER` | LOGIN |
| 1011 | `LOGIN_SUCCESS_MANAGER` | LOGIN |
| 1012 | `LOGIN_SUCCESS_ADMIN` | LOGIN |
| 2011 | `LOGIN_FAIL` | LOGIN |
| 2012 | `LOGIN_ALREADY` | LOGIN |
| 1030 | `LOGOUT_SUCCESS` | LOGOUT, EXIT |
| 1020 | `REGISTER_SUCCESS` | REGISTER |
| 2021 | `REGISTER_FAIL` | REGISTER |
| 1110 | `CHANGE_PASSWORD_SUCCESS` | CHANGE_PASSWORD |
| 2110 | `CHANGE_PASSWORD_FAIL` | CHANGE_PASSWORD |
| 1040 | `ADD_FILM_SUCCESS` | ADD_FILM |
| 2041 | `ADD_FILM_FAIL` | ADD_FILM |
| 1050 | `POST_FILM_SUCCESS` | Chưa phát qua handler đang chạy |
| 2051 | `POST_FILM_FAIL` | Chưa phát qua handler đang chạy |
| 1060 | `EDIT_FILM_SUCCESS` | Chưa phát qua handler đang chạy |
| 2061 | `EDIT_FILM_FAIL` | Chưa phát qua handler đang chạy |
| 2062 | `NO_EDIT_FILM` | Chưa phát qua handler đang chạy |
| 2063 | `FILM_EXISTS` | Chưa phát qua handler đang chạy |
| 1073 | `FIND_FILM_SUCCESS` | SHOW_FILMS, SEARCH_FILM_BY_TITLE |
| 2072 | `FIND_FILM_FAIL` | SEARCH_FILM_BY_TITLE |
| 1070 | `BROWSE_CATEGORY_SUCCESS` | BROWSE_CATEGORY |
| 1071 | `BROWSE_THEATER_SUCCESS` | SHOW_CINEMAS, BROWSE_CINEMA, SHOW_CINEMA_BY_FILM |
| 1072 | `BROWSE_TIME_SUCCESS` | SHOW_PREMIERED_TIME, BROWSE_PREMIERED_TIME, SHOW_TIME_BY_FILM_CINEMA |
| 2071 | `BROWSE_FAIL` | BROWSE_CATEGORY, BROWSE_CINEMA, BROWSE_PREMIERED_TIME |
| 1065 | `SHOW_CATEGORY_SUCCESS` | SHOW_CATEGORIES |
| 2060 | `SHOW_CATEGORY_FAIL` | Chưa phát qua handler đang chạy |
| 1080 | `VIEW_CHAIR_SUCCESS` | SHOW_SEATS (3 trạng thái), VIEW_TICKETS, VIEW_TICKET_DETAIL, SHOW_ROOMS_BY_CINEMA |
| 2081 | `VIEW_CHAIR_FAIL` | VIEW_TICKET_DETAIL |
| 1091 | `SHOW_SHOWTIME_SUCCESS` | SHOW_SHOWTIMES_BY_ROOM |
| 2092 | `SHOW_TIME_FAIL` | SHOW_SHOWTIMES_BY_ROOM |
| 3001 | `CANT_LOAD_CATEGORIES` | SHOW_CATEGORIES |
| 3002 | `NO_CATEGORIES` | SHOW_CATEGORIES |
| 3003 | `CANT_LOAD_CINEMAS` | SHOW_CINEMAS, SHOW_CINEMA_BY_FILM |
| 3004 | `NO_CINEMAS` | SHOW_CINEMAS, SHOW_CINEMA_BY_FILM |
| 3005 | `CANT_LOAD_SHOWTIMES` | SHOW_PREMIERED_TIME, SHOW_TIME_BY_FILM_CINEMA |
| 3006 | `NO_SHOWTIMES` | SHOW_PREMIERED_TIME, SHOW_TIME_BY_FILM_CINEMA |
| 3007 | `CANT_LOAD_FILMS` | SHOW_FILMS |
| 3008 | `NO_FILMS` | SHOW_FILMS |
| 3009 | `CANT_LOAD_SEATS` | Chưa phát qua handler đang chạy |
| 3010 | `NO_SEATS` | Chưa phát qua handler đang chạy |
| 3011 | `CANT_LOAD_ROOMS` | Chưa phát qua handler đang chạy |
| 3012 | `NO_ROOMS` | SHOW_ROOMS_BY_CINEMA |
| 3013 | `NO_TICKETS` | VIEW_TICKETS |
| 3014 | `FAILED_RETRIEVE` | VIEW_TICKETS |
| 3015 | `TICKET_NOT_FOUND` | VIEW_TICKET_DETAIL |
| 3016 | `INVALID_CINEMA` | SHOW_ROOMS_BY_CINEMA |
| 3017 | `FAILED_ROOMS` | SHOW_ROOMS_BY_CINEMA |
| 3018 | `INVALID_REQUEST` | Dispatcher / handlers |
| 3019 | `INVALID_SHOWTIME` | SHOW_SEATS, BOOK_TICKETS |
| 3020 | `NO_SEATS_AVAILABLE` | Không còn dùng (SHOW_SEATS trả mọi ghế kèm trạng thái) |
| 1090 | `CHOOSE_CHAIR_SUCCESS` | Chưa phát qua handler đang chạy |
| 2091 | `CHOOSE_CHAIR_FAIL` | Chưa phát qua handler đang chạy |
| 1200 | `BOOK_TICKET_SUCCESS` | BOOK_TICKET |
| 1210 | `BOOK_TICKETS_SUCCESS` | BOOK_TICKETS |
| 1220 | `PAY_BOOKING_SUCCESS` | PAY_BOOKING |
| 1230 | `CANCEL_BOOKING_SUCCESS` | CANCEL_BOOKING |
| 2200 | `BOOK_TICKET_FAIL` | BOOK_TICKET |
| 3021 | `INVALID_FILM_ID` | ADD_SHOWTIME |
| 3022 | `DB_QUERY_ERROR` | LOGIN, SHOW_ROOMS_BY_CINEMA, ADD_SHOWTIME, refresh_role |
| 3023 | `INVALID_CINEMA_ID` | ADD_SHOWTIME |
| 3024 | `INVALID_ROOM_ID` | ADD_SHOWTIME |
| 3025 | `ROOM_NOT_BELONG_CINEMA` | ADD_SHOWTIME |
| 3026 | `INVALID_DATETIME_FORMAT` | ADD_SHOWTIME |
| 3027 | `SCHEDULING_CONFLICT` | ADD_SHOWTIME |
| 1092 | `ADD_SHOWTIME_SUCCESS` | ADD_SHOWTIME |
| 2090 | `ADD_SHOWTIME_FAIL` | ADD_SHOWTIME |
| 1300 | `SHOW_ALL_USERS_SUCCESS` | SHOW_ALL_USERS |
| 2300 | `SHOW_ALL_USERS_FAIL` | SHOW_ALL_USERS |
| 1301 | `DELETE_USER_SUCCESS` | DELETE_USER |
| 2301 | `DELETE_USER_FAIL` | DELETE_USER |
| 1302 | `UPDATE_USER_ROLE_SUCCESS` | CHANGE_USER_ROLE |
| 2302 | `UPDATE_USER_ROLE_FAIL` | CHANGE_USER_ROLE |
| 3028 | `AUTH_REQUIRED` | Dispatcher / refresh_role |
| 3029 | `PERMISSION_DENIED` | Dispatcher |
| 3030 | `RESPONSE_TOO_LARGE` | Handlers tạo danh sách hoặc chi tiết |
| 3031 | `SERVER_BUSY` | Không mượn được kết nối CSDL (pool 10 kết nối, chờ 2 giây), hoặc BOOK_TICKET/BOOK_TICKETS/PAY_BOOKING không lấy được khóa suất chiếu sau 30 giây. Chờ khóa suất chiếu không chiếm kết nối. Client có thể gửi lại trên cùng socket. |
| 3032 | `SEAT_TAKEN` | BOOK_TICKETS, PAY_BOOKING |
| 3033 | `PAY_ALREADY_DONE` | PAY_BOOKING |
| 3034 | `BOOKING_EXPIRED` | PAY_BOOKING |
| 3035 | `NOT_ENOUGH_BALANCE` | PAY_BOOKING |
| 3036 | `BOOKING_NOT_FOUND` | PAY_BOOKING, CANCEL_BOOKING |
| 3037 | `INVALID_SEAT_LIST` | BOOK_TICKETS |
| 3038 | `BOOKING_NOT_PENDING` | CANCEL_BOOKING |

## 8. Security / Session Rules

- Lệnh đã biết cần đăng nhập bị trả 3028 nếu gọi từ khách. Khi đã đăng nhập, lệnh sai vai trò bị trả 3029; LOGIN lặp lại bị trả 2012. Client cần giữ cùng socket sau LOGIN.
- Dùng danh tính phiên để truy cập vé và cập nhật mật khẩu. VIEW_TICKET_DETAIL ràng buộc cả ticket_id lẫn user_id của phiên.
- Đường xử lý hiện tại log mã kết quả, hướng và FD. Giữ mật khẩu và payload xác thực ngoài log khi mở rộng.
- Transport hiện là TCP thuần. Code xác thực so sánh mật khẩu với giá trị trong CSDL và lưu trực tiếp khi đăng ký hoặc đổi mật khẩu. Cần xác nhận phương án bảo vệ mật khẩu trước khi triển khai ngoài môi trường học tập.

## 9. Open Issues / Protocol Decisions Pending

- **Cao Anh/Minh xác nhận encoding.** Qt dùng UTF-8 nhưng lớp mạng và kết nối DB không ấn định hoặc xác thực UTF-8.
- **Cao Anh/Minh xác nhận mã và payload.** README chỉ liệt kê nhóm tài khoản. Một số tên mã được tái sử dụng cho nhiều loại dữ liệu (1071, 1072, 1073, 1080). 2071, 2072 và 2092 gộp trường hợp rỗng và lỗi truy vấn. Chốt mã theo lệnh trước khi client bổ sung xử lý lỗi riêng.
- **Đồng bộ client.** Header message còn khai báo makeAnnounceFilmMessage, trong khi dispatcher không có lệnh công bố phim. Console in trạng thái logout sau khi đọc mã nhưng bỏ qua giá trị trả về.
- **Cache suất chiếu.** Đã bỏ (tuần 7). SHOW_SEATS và đặt vé đọc thẳng CSDL; suất thêm bằng ADD_SHOWTIME đặt được ngay, không cần khởi động lại server.
- **Seed mẫu.** Dòng tài khoản longmh trong `db/data.txt` thiếu dấu nháy đóng tên hiển thị. Cần xác nhận seed trước khi dùng các tài khoản README cho regression.
- **Sprint 2 còn lại.** Hủy vé đã thanh toán có hoàn tiền, VIEW_PAYMENTS, nạp tiền ví, PING/PONG và giới hạn client là việc của tuần 7-8 theo kế hoạch; chưa có trong dispatcher.

### Chạy regression

Sau khi server đã chạy và CSDL có tài khoản User, chạy `python3 tests/regression_test.py --host 127.0.0.1 --port 8080 --timeout 5`. Mặc định dùng tài khoản mẫu `user1` / `pass123`. Có thể đặt `MOVIEGO_TEST_USER` và `MOVIEGO_TEST_USER_PASSWORD` để đổi tài khoản. Đặt cặp `MOVIEGO_TEST_MANAGER` / `MOVIEGO_TEST_MANAGER_PASSWORD` và `MOVIEGO_TEST_ADMIN` / `MOVIEGO_TEST_ADMIN_PASSWORD` để bật các bài tương ứng. Bài Manager cần ít nhất một rạp. Nên dùng tài khoản kiểm thử đang rảnh vì server từ chối đăng nhập đồng thời.

Script kiểm tra server qua TCP và dùng truy vấn đọc cùng các thao tác phiên. `tests/test_week6.py` vẫn là bộ tích hợp riêng, tự dựng MariaDB tạm và kiểm tra thêm các lệnh ghi dữ liệu.
