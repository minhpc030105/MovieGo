"""Run on Linux with make, GCC, libmysqlclient-dev and MariaDB server/client tools."""
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from pathlib import Path
from tempfile import TemporaryDirectory
import os
import getpass
import socket
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def frame(text):
    data = text.encode()
    return struct.pack("!I", len(data)) + data


def read_exact(peer, length):
    data = bytearray()
    while len(data) < length:
        chunk = peer.recv(length - len(data))
        assert chunk, "connection ended inside a frame"
        data.extend(chunk)
    return bytes(data)


def response(peer):
    length, = struct.unpack("!I", read_exact(peer, 4))
    assert 0 < length <= 65536
    return read_exact(peer, length).decode().split("\r\n")


def request(peer, *lines):
    peer.sendall(frame("\r\n".join(lines) + "\r\n"))
    return response(peer)


@contextmanager
def running(command, cwd, env=None):
    with (cwd / (Path(command[0]).name + ".log")).open("w+") as log:
        proc = subprocess.Popen(command, cwd=cwd, env=env, stdout=log, stderr=log)
        try:
            yield proc
        except BaseException:
            log.seek(0)
            print(log.read()[-3000:])
            raise
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()


def wait_until(check, process):
    for _ in range(150):
        if process.poll() is not None:
            raise RuntimeError(f"{process.args[0]} exited with {process.returncode}")
        if check():
            return
        time.sleep(0.1)
    raise TimeoutError(process.args[0])


def exercise(port, sql, client):
    def connect():
        return socket.create_connection(("127.0.0.1", port), timeout=4)

    with connect() as alice:
        packet = frame("LOGIN\r\nalice\r\npw\r\n")
        for byte in packet:
            alice.sendall(bytes([byte]))
            time.sleep(0.001)
        assert response(alice) == ["1010"]
        with connect() as failed:
            assert request(failed, "LOGIN", "alice", "wrong") == ["2011"]
        with connect() as second:
            assert request(second, "LOGIN", "alice", "pw") == ["2012"]
        with connect() as guest:
            assert request(guest, "SHOW_ALL_USERS") == ["3028"]
        alice.sendall(frame("SHOW_FILMS") + frame("SHOW_ALL_USERS"))
        assert response(alice) == ["1073", "1. First", "2. Second"]
        assert response(alice) == ["3029"]
        assert request(alice, "VIEW_TICKETS") == ["3013"]
        assert request(alice, "SHOW_CINEMA_BY_FILM", "1") == ["1071", "1. Cinema"]
        assert request(alice, "SHOW_TIME_BY_FILM_CINEMA", "1", "1")[0] == "1072"
        assert request(alice, "SHOW_PREMIERED_TIME") == ["1072", "10:00 - 11:30"]
        assert request(alice, "BROWSE_PREMIERED_TIME", "10:00")[0] == "1072"
        assert request(alice, "BROWSE_CATEGORY", "1")[0] == "1070"
        assert request(alice, "BROWSE_CINEMA", "1")[0] == "1071"
        assert request(alice, "SEARCH_FILM_BY_TITLE", "Firs")[0] == "1073"
        assert request(alice, "ADD_FILM", "Third", "1", "90") == ["3029"]
        assert request(alice, "SHOW_SEATS", "1") == ["1080", "SEAT|1|A-1"]
        assert request(alice, "BOOK_TICKET", "1", "1") == ["1200"]
        assert request(alice, "VIEW_TICKET_DETAIL", "1")[1] == "=== TICKET DETAIL ==="
        assert request(alice, "BOOK_TICKET", "1", "") == ["3018"]
        alice.sendall(frame("X" * 65536))
        assert response(alice) == ["3018"]
        assert request(alice, "LOGOUT") == ["1030"]

    with connect() as manager, connect() as admin:
        assert request(manager, "LOGIN", "manager", "pw") == ["1011"]
        assert request(admin, "LOGIN", "admin", "pw") == ["1012"]
        assert request(manager, "ADD_FILM", "Third", "1", "90") == ["1040"]
        assert request(manager, "DELETE_USER", "1") == ["3029"]
        assert request(manager, "SHOW_ROOMS_BY_CINEMA", "1")[0] == "1080"
        assert request(manager, "ADD_SHOWTIME", "1", "1", "1", "2030-01-01 13:00:00") == ["1092"]
        assert request(manager, "ADD_SHOWTIME", "1", "1", "1", "2030-01-01 13:00:00") == ["3027"]
        assert request(manager, "SHOW_SHOWTIMES_BY_ROOM", "1")[0] in ("1091", "2092")
        assert request(admin, "SHOW_ALL_USERS")[0] == "1300"
        assert request(admin, "CHANGE_USER_ROLE", "2", "2") == ["1302"]
        assert request(manager, "ADD_FILM", "Fourth", "1", "90") == ["3029"]
        assert request(manager, "VIEW_TICKET_DETAIL", "1") == ["3015"]

    def independent_login(i):
        with connect() as user:
            name = f"week6-{i}"
            assert request(user, "REGISTER", name, name, "pw") == ["1020"]
            assert request(user, "LOGIN", name, "pw") == ["1010"]
            assert request(user, "CHANGE_PASSWORD", "pw", "changed") == ["1110"]
            assert request(user, "LOGOUT") == ["1030"]
            assert request(user, "LOGIN", name, "changed") == ["1010"]

    with ThreadPoolExecutor(max_workers=8) as pool:
        list(pool.map(independent_login, range(16)))

    with connect() as user:
        assert request(user, "LOGIN", "alice", "pw") == ["1010"]
        # Disconnect in the middle of the next body.
        user.sendall(struct.pack("!I", 20) + b"SHOW")
    with connect() as user:
        for _ in range(50):
            code = request(user, "LOGIN", "alice", "pw")
            if code != ["2012"]:
                break
            time.sleep(0.01)
        assert code == ["1010"]
        sql("INSERT INTO films(title,category_id,show_time) "
            "SELECT REPEAT('f',250),1,90 FROM seq_1_to_300;")
        assert request(user, "SHOW_FILMS") == ["3030"]
        assert request(user, "SHOW_CATEGORIES") == ["1065", "1. Drama"]
        # Construct a response exactly at the byte limit, then grow it by one.
        remaining = 65536 - len("1065\r\n1. Drama")
        values = []
        category = 2
        while remaining:
            prefix = f"\r\n{category}. "
            count = min(250, remaining - len(prefix))
            assert count > 0
            values.append(f"({category},'{('a' * count)}')")
            remaining -= len(prefix) + count
            category += 1
        sql("INSERT INTO categories(id,name) VALUES " + ",".join(values))
        boundary = request(user, "SHOW_CATEGORIES")
        assert boundary[0] == "1065" and len("\r\n".join(boundary)) == 65536
        sql("UPDATE categories SET name='Dramas' WHERE id=1")
        assert request(user, "SHOW_CATEGORIES") == ["3030"]
        assert request(user, "EXIT") == ["1030"]
        assert user.recv(1) == b""

    with connect() as peer:
        peer.sendall(struct.pack("!I", 65537))
        assert peer.recv(1) == b""

    result = subprocess.run([client, "127.0.0.1", str(port)], input="1\nalice\npw\n5\n3\n",
                            text=True, capture_output=True, check=True, timeout=5)
    assert "logged in successfully" in result.stdout and "logged out" in result.stdout


def main():
    with TemporaryDirectory(prefix="moviego-week6-") as tmp:
        work = Path(tmp)
        server, client = work / "server_out", work / "client_out"
        subprocess.run(["make", f"OUTPUT_SERVER={server}", f"OUTPUT_CLIENT={client}"], cwd=ROOT, check=True)
        socket_test = work / "test_socket"
        subprocess.run(["cc", "-Wall", "-pthread", "tests/test_socket.c", "lib/socket/socket.c",
                        "-o", socket_test], cwd=ROOT, check=True)
        subprocess.run([socket_test], check=True, timeout=5)
        data, sock = work / "data", str(work / "mysql.sock")
        subprocess.run(["mariadb-install-db", "--no-defaults", f"--datadir={data}",
                        "--auth-root-authentication-method=normal"], stdout=subprocess.DEVNULL, check=True)
        env = dict(os.environ, MYSQL_UNIX_PORT=sock)
        db_command = ["mariadbd", "--no-defaults", f"--datadir={data}", f"--socket={sock}",
                      f"--user={getpass.getuser()}", "--skip-networking", f"--pid-file={work / 'mysql.pid'}"]
        with running(db_command, work) as db:
            cli = ["mariadb", "--no-defaults", "--protocol=socket", f"--socket={sock}", "-uroot"]
            wait_until(lambda: subprocess.run(cli + ["-e", "SELECT 1"], capture_output=True).returncode == 0, db)

            def sql(statement):
                subprocess.run(cli, input=statement, text=True, check=True)

            sql((ROOT / "db/booking.sql").read_text())
            cli.append("booking")
            sql("INSERT INTO users(name,username,password,role_id) VALUES "
                "('Alice','alice','pw',2),('Manager','manager','pw',1),('Admin','admin','pw',0);"
                "INSERT INTO categories(name) VALUES ('Drama');"
                "INSERT INTO films(title,category_id,show_time) VALUES ('First',1,90),('Second',1,90);"
                "INSERT INTO cinemas(name,address) VALUES ('Cinema','Street');"
                "INSERT INTO rooms(name,cinema_id,total_seats) VALUES ('Room',1,1);"
                "INSERT INTO seats(room_id,seat_number,row_name) VALUES (1,'1','A');"
                "INSERT INTO showtimes(film_id,room_id,start_time,end_time) "
                "VALUES (1,1,'2030-01-01 10:00','2030-01-01 11:30');"
                "ALTER USER 'root'@'localhost' IDENTIFIED BY '123456';")
            cli.append("-p123456")
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 0))
                port = probe.getsockname()[1]

            def listening():
                with socket.socket() as probe:
                    return probe.connect_ex(("127.0.0.1", port)) == 0

            with running([str(server), str(port)], work, env) as proc:
                wait_until(listening, proc)
                exercise(port, sql, str(client))
    print("Week 6 framing, session, role and Console checks passed.")


if __name__ == "__main__":
    main()
