"""Week 7 live test: multi-seat hold, wallet payment, cancel, and the hot-showtime scenario.

Needs a RUNNING server and the v2 database (booking_v2.sql, or upgrade_v2.sql + upgrade_week7.sql):
    ./server_out 8080
    python3 tests/test_week7_booking.py --port 8080 --showtime 1 --other-showtime 4

The server allows one login per account, so the 100-client part registers
w7load000..w7load119 (100 hot + 20 side accounts) (password 'pw') on first run and reuses them afterwards.
Every client must end with a result code: 1210 (held) or 3032 (seat taken). 3031 (SERVER_BUSY)
on the hot showtime or on the other showtime FAILS the test: that is the starvation bug.
The test uses the seats in --seats (default 91..99 of the showtime room) and leaves bookings
PENDING; run it on a test database, they expire after 10 minutes or can be cancelled.
"""
import argparse
import socket
import struct
import threading
import time
from concurrent.futures import ThreadPoolExecutor


def frame(*lines):
    data = "\r\n".join(lines).encode()
    return struct.pack("!I", len(data)) + data


def recv_exact(peer, length):
    data = bytearray()
    while len(data) < length:
        chunk = peer.recv(length - len(data))
        if not chunk:
            raise ConnectionError("connection closed inside a frame")
        data.extend(chunk)
    return bytes(data)


def request(peer, *lines):
    peer.sendall(frame(*lines))
    length, = struct.unpack("!I", recv_exact(peer, 4))
    assert 0 < length <= 65536, length
    return recv_exact(peer, length).decode().split("\r\n")


class Client:
    def __init__(self, host, port, timeout):
        self.peer = socket.create_connection((host, port), timeout=timeout)

    def ask(self, *lines):
        return request(self.peer, *lines)

    def login(self, user, password):
        return self.ask("LOGIN", user, password)

    def close(self):
        try:
            request(self.peer, "EXIT")
        except Exception:
            pass
        self.peer.close()


def seat_states(reply):
    assert reply[0] == "1080", reply[:2]
    states = {}
    for line in reply[1:]:
        tag, seat_id, name, state = line.split("|")
        assert tag == "SEAT" and state in ("AVAILABLE", "HELD", "BOOKED"), line
        states[int(seat_id)] = state
    return states


def parse_booking(line):
    tag, booking_id, status, total, *rest = line.split("|")
    assert tag == "BOOKING"
    return int(booking_id), status, total, rest


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("--showtime", type=int, default=1, help="the popular (hot) showtime")
    ap.add_argument("--other-showtime", type=int, default=4, help="a different showtime that must never be slowed")
    ap.add_argument("--user", default="user")
    ap.add_argument("--password", default="123456")
    ap.add_argument("--clients", type=int, default=100)
    args = ap.parse_args()

    def connect():
        return Client(args.host, args.port, args.timeout)

    passed = []

    def check(name, condition, detail=""):
        assert condition, f"FAIL {name} {detail}"
        passed.append(name)
        print("PASS", name)

    # ---- single user: states, multi-seat all-or-nothing, pay, cancel -------------------------
    me = connect()
    check("login", me.login(args.user, args.password) == ["1010"])
    states = seat_states(me.ask("SHOW_SEATS", str(args.showtime)))
    free = [s for s, st in sorted(states.items()) if st == "AVAILABLE"]
    assert len(free) >= 12, "need at least 12 free seats in the hot showtime for this test"
    a, b, c, d, e = free[-5:]
    check("show_seats_3_states", set(states.values()) <= {"AVAILABLE", "HELD", "BOOKED"})
    check("bad_seat_list_rejected", all(me.ask("BOOK_TICKETS", str(args.showtime), bad) == ["3037"]
                                        for bad in ("1,,2", "1 ,2", "x", "0", f"{a},{a}", ",".join(str(i) for i in range(1, 12)))))
    check("unknown_showtime", me.ask("BOOK_TICKETS", "999999", str(a)) == ["3019"])
    check("seat_of_other_room_rejected", me.ask("BOOK_TICKETS", str(args.showtime), "999999") == ["3037"])

    reply = me.ask("BOOK_TICKETS", str(args.showtime), f"{a},{b}")
    check("hold_two_seats", reply[0] == "1210" and len(reply) == 2, reply)
    booking, status, total, _ = parse_booking(reply[1])
    check("status_pending_and_server_total", status == "PENDING" and float(total) > 0)
    states = seat_states(me.ask("SHOW_SEATS", str(args.showtime)))
    check("seats_now_held", states[a] == states[b] == "HELD")

    # all-or-nothing: one held seat in the order rejects the whole order
    reply = me.ask("BOOK_TICKETS", str(args.showtime), f"{c},{a}")
    check("whole_order_rejected", reply == ["3032"], reply)
    check("rejected_order_holds_nothing", seat_states(me.ask("SHOW_SEATS", str(args.showtime)))[c] == "AVAILABLE")

    # cancel, then the seats are free at once
    check("cancel_unpaid", me.ask("CANCEL_BOOKING", str(booking)) == ["1230"])
    check("cancel_twice_rejected", me.ask("CANCEL_BOOKING", str(booking)) == ["3038"])
    check("cancel_unknown", me.ask("CANCEL_BOOKING", "999999999") == ["3036"])
    check("pay_cancelled_rejected", me.ask("PAY_BOOKING", str(booking)) == ["3034"])
    check("seats_free_after_cancel", seat_states(me.ask("SHOW_SEATS", str(args.showtime)))[a] == "AVAILABLE")

    # pay: balance goes down once, second pay never charges again
    reply = me.ask("BOOK_TICKETS", str(args.showtime), f"{d}")
    booking2, _, total2, _ = parse_booking(reply[1])
    paid = me.ask("PAY_BOOKING", str(booking2))
    check("pay_ok", paid[0] == "1220", paid)
    _, pstatus, ptotal, rest = parse_booking(paid[1])
    balance_after = float(rest[0])
    check("paid_status", pstatus == "PAID" and ptotal == total2)
    check("pay_twice_no_second_charge", me.ask("PAY_BOOKING", str(booking2)) == ["3033"])
    check("seat_booked_after_pay", seat_states(me.ask("SHOW_SEATS", str(args.showtime)))[d] == "BOOKED")
    check("old_book_ticket_cannot_take_booked_seat", me.ask("BOOK_TICKET", str(args.showtime), str(d)) == ["2200"])
    check("pay_unknown_booking", me.ask("PAY_BOOKING", "999999999") == ["3036"])
    check("balance_not_negative", balance_after >= 0)
    me.close()

    # ---- the hot showtime: N clients, ONE seat, plus a client on another showtime -----------------
    n = args.clients
    SIDE = 20                                            # extra accounts for the other showtime
    names = [f"w7load{i:03d}" for i in range(n + SIDE)]
    for name in names:                                   # idempotent: 2021 means it already exists
        c0 = connect()
        r = c0.ask("REGISTER", name, name, "pw")
        assert r[0] in ("1020", "2021"), r
        c0.close()
    clients = []
    for name in names:
        cl = connect()
        assert cl.login(name, "pw") == ["1010"], name
        clients.append(cl)

    # a seat nobody holds
    seat = e
    other_states = seat_states(clients[n].ask("SHOW_SEATS", str(args.other_showtime)))
    other_free = [s for s, st in sorted(other_states.items()) if st == "AVAILABLE"]
    assert len(other_free) >= SIDE, f"the other showtime needs {SIDE} free seats"

    start = threading.Barrier(n + SIDE)
    results, latencies = [None] * n, [0.0] * n
    other_results, other_latency = [], []

    def hot(i):
        start.wait()
        t0 = time.monotonic()
        results[i] = clients[i].ask("BOOK_TICKETS", str(args.showtime), str(seat))
        latencies[i] = time.monotonic() - t0

    def side(i):
        # SIDE bookings (one distinct seat each) on a DIFFERENT showtime while the hot one is saturated
        start.wait()
        t0 = time.monotonic()
        other_results.append(clients[n + i].ask("BOOK_TICKETS", str(args.other_showtime), str(other_free[i])))
        other_latency.append(time.monotonic() - t0)

    t0 = time.monotonic()
    with ThreadPoolExecutor(max_workers=n + SIDE) as pool:
        futures = [pool.submit(hot, i) for i in range(n)] + [pool.submit(side, i) for i in range(SIDE)]
        for f in futures:
            f.result()
    total_time = time.monotonic() - t0

    codes = [r[0] for r in results]
    ok, taken, busy = codes.count("1210"), codes.count("3032"), codes.count("3031")
    print(f"hot showtime: {ok} held, {taken} seat taken, {busy} busy, total {total_time:.2f}s, "
          f"slowest {max(latencies):.2f}s; other showtime slowest {max(other_latency):.3f}s")
    check("hot_exactly_one_winner", ok == 1, codes)
    check("hot_everyone_else_seat_taken", taken == n - 1, codes)
    check("hot_no_server_busy", busy == 0, f"{busy} requests got SERVER_BUSY")
    check("other_showtime_all_booked", all(r[0] == "1210" for r in other_results),
          [r[0] for r in other_results])
    check("other_showtime_fast", max(other_latency) < 2.0, f"{max(other_latency):.2f}s")

    # the winner pays, then everyone still logged in can log out
    winner = codes.index("1210")
    wid, _, _, _ = parse_booking(results[winner][1])
    check("winner_can_pay", clients[winner].ask("PAY_BOOKING", str(wid))[0] == "1220")
    for cl in clients:
        cl.close()
    print(f"\nALL {len(passed)} CHECKS PASSED")


if __name__ == "__main__":
    main()
