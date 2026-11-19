# Giao thức MovieGo · Tuần 6

## Khung TCP

Cả hai chiều dùng `[LENGTH][PAYLOAD]`. LENGTH là số nguyên không dấu 4 byte theo thứ tự mạng (big-endian), đếm số byte của PAYLOAD. Payload là văn bản UTF-8 dài từ 1 đến 65.536 byte. Byte NUL kết thúc chuỗi trong C nằm ngoài khung.

Client gửi lệnh đầu tiên ngay sau khi kết nối. Mỗi request hợp lệ về khung nhận đúng một response. Dòng đầu của request là tên lệnh viết hoa, mỗi tham số nằm trên một dòng tiếp theo. Dấu phân cách là `\r\n`. Có thể thêm một `\r\n` cuối request. Mỗi tham số tối đa 255 byte, các trường bắt buộc phải có giá trị.

Dòng đầu của response là mã kết quả gồm 4 chữ số ASCII. Các dòng tiếp theo chứa dữ liệu. Client đọc đủ LENGTH byte rồi duyệt các dòng trong bộ nhớ. Ví dụ, request `SHOW_FILMS\r\n` có LENGTH bằng 12. Một response có payload `1073\r\n1. First\r\n2. Second` có LENGTH bằng 25.

Khi độ dài khung bằng 0, vượt 65.536, payload chứa NUL, hoặc kết nối đứt giữa khung, server đóng socket và giải phóng phiên. Lệnh sai cú pháp trong một khung hợp lệ nhận `3018`. Nếu toàn bộ phản hồi (kể cả mã và CRLF) vượt 65.536 byte, client chỉ nhận `3030` và có thể tiếp tục gửi lệnh. Timeout nhận là 30 phút không có dữ liệu. `EXIT` trả `1030` rồi đóng kết nối.

## Lệnh và quyền

| Đối tượng | Lệnh và tham số theo thứ tự |
| --- | --- |
| Chưa đăng nhập | `REGISTER name username password`, `LOGIN username password` |
| Mọi kết nối | `EXIT` |
| Mọi tài khoản đã đăng nhập | `LOGOUT`, `CHANGE_PASSWORD old_password new_password` |
| User hoặc Manager | `SHOW_CATEGORIES`, `SHOW_CINEMAS`, `SHOW_PREMIERED_TIME`, `SHOW_FILMS`, `BROWSE_CATEGORY category_id`, `BROWSE_CINEMA cinema_id`, `BROWSE_PREMIERED_TIME HH:MM`, `SEARCH_FILM_BY_TITLE title` |
| User | `SHOW_CINEMA_BY_FILM film_id`, `SHOW_TIME_BY_FILM_CINEMA film_id cinema_id`, `SHOW_SEATS showtime_id`, `BOOK_TICKET showtime_id seat_id`, `VIEW_TICKETS`, `VIEW_TICKET_DETAIL ticket_id` |
| Manager | `ADD_FILM title category_id duration_minutes`, `SHOW_ROOMS_BY_CINEMA cinema_id`, `SHOW_SHOWTIMES_BY_ROOM room_id`, `ADD_SHOWTIME film_id cinema_id room_id start_datetime` |
| Admin | `SHOW_ALL_USERS`, `DELETE_USER user_id`, `CHANGE_USER_ROLE user_id role_id` |

Khoảng trắng trong bảng phân tách tham số cho dễ đọc. Khi gửi qua mạng, mỗi tham số chiếm một dòng, kể cả tiêu đề phim hoặc ngày giờ có khoảng trắng. Các vai trò trong CSDL là Admin (0), Manager (1) và User (2). Phiên gắn với kết nối đã đăng nhập thành công. Server dùng danh tính trong phiên khi đổi mật khẩu, đặt vé hoặc xem vé.

## Mã kết quả

Danh sách mã hiện hành ở `lib/resultCodes.h`. Các mã được bổ sung cho nhóm gồm các trường hợp sau.

| Mã | Tên | Ý nghĩa |
| --- | --- | --- |
| 3028 | AUTH_REQUIRED | Cần đăng nhập |
| 3029 | PERMISSION_DENIED | Vai trò hoặc trạng thái phiên không cho phép lệnh |
| 3030 | RESPONSE_TOO_LARGE | Phản hồi vượt giới hạn |
| 3031 | SERVER_BUSY | Dành cho trường hợp hết thời gian chờ kết nối CSDL |
| 3032 | SEAT_TAKEN | Dành cho ghế đã được giữ hoặc bán |
| 3033 | PAY_ALREADY_DONE | Dành cho đơn đã thanh toán |
| 3034 | BOOKING_EXPIRED | Dành cho đơn hết hạn |
| 3035 | NOT_ENOUGH_BALANCE | Dành cho ví thiếu tiền |

## Thêm handler

Khai báo theo kiểu `Handler` trong `server/headers/function.h`, viết hàm trong module nghiệp vụ tương ứng và đăng ký tên lệnh cùng quyền trong `command_table` ở `dispatch.c`. Handler nhận `Session *`, chuỗi tham số, bộ đệm thân phản hồi và dung lượng còn lại. Trả mã kết quả dưới dạng `int`. Với phản hồi nhiều dòng, dùng `append_line` và trả `RESPONSE_TOO_LARGE` nếu thêm dòng thất bại. Lớp điều phối ghép mã với dữ liệu. Lớp mạng gửi khung sau khi handler đã trả về.

Chạy `python3 tests/test_week6.py` từ thư mục gốc trên Linux để kiểm tra giao thức và phiên bằng MariaDB tạm. Bài kiểm tra cần Python 3, Make, GCC, thư viện client MySQL và công cụ MariaDB (`mariadbd`, `mariadb-install-db`, `mariadb`).
