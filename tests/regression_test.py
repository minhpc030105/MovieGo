"""Read-only TCP regression suite for a running MovieGo server."""

import argparse
from contextlib import contextmanager
import math
import os
import re
import socket
import struct
from uuid import uuid4

MAX_FRAME = 65536
ACCOUNTS = {
    "USER": ("user1", "pass123", "1010"),
    "MANAGER": (None, None, "1011"),
    "ADMIN": (None, None, "1012"),
}


class TestFailure(Exception):
    pass


class SkipTest(Exception):
    pass


def frame(*lines):
    payload = "\r\n".join(lines).encode("utf-8")
    if not 0 < len(payload) <= MAX_FRAME:
        raise ValueError("request size outside protocol bounds")
    return struct.pack("!I", len(payload)) + payload


def recv_exact(peer, length):
    data = bytearray()
    while len(data) < length:
        chunk = peer.recv(length - len(data))
        if not chunk:
            raise TestFailure(f"EOF after {len(data)} of {length} expected bytes")
        data.extend(chunk)
    return bytes(data)


def recv_message(peer):
    (length,) = struct.unpack("!I", recv_exact(peer, 4))
    if not 0 < length <= MAX_FRAME:
        raise TestFailure(f"invalid response length {length}")
    data = recv_exact(peer, length)
    # The server does not enforce a charset. Match only ASCII wire syntax.
    lines = data.decode("ascii", errors="replace").split("\r\n")
    if b"\x00" in data or any(
        "\r" in line or "\n" in line for line in lines
    ):
        raise TestFailure("invalid response text framing")
    if re.fullmatch(r"[0-9]{4}", lines[0]) is None:
        raise TestFailure("expected a four-digit response code")
    return lines[0], lines[1:]


def expect(reply, success, empty=None, pattern=None):
    code, lines = reply
    if empty is not None and code == empty and not lines:
        return lines
    if code != success:
        expected = success if empty is None else f"{success} or {empty}"
        raise TestFailure(f"expected {expected}, got {code}")
    if pattern is None:
        if lines:
            raise TestFailure(f"expected code-only response {success}, got a body")
    elif not lines or any(re.fullmatch(pattern, line) is None for line in lines):
        raise TestFailure(f"unexpected body format for response {success}")
    return lines


def request(peer, expected, *lines, empty=None, pattern=None):
    peer.sendall(frame(*lines))
    return expect(recv_message(peer), expected, empty, pattern)


def run_test(name, function):
    try:
        function()
    except SkipTest as exc:
        print(f"[SKIP] {name}: {exc}")
        return "Skipped"
    except (TestFailure, OSError) as exc:
        print(f"[FAIL] {name}: {exc}")
        return "Failed"
    print(f"[PASS] {name}")
    return "Passed"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("port must be 1..65535 and timeout must be finite and positive")

    accounts = {}
    for role, (default_user, default_password, code) in ACCOUNTS.items():
        username = os.getenv(f"MOVIEGO_TEST_{role}", default_user)
        password = os.getenv(f"MOVIEGO_TEST_{role}_PASSWORD", default_password)
        if username is None and password is None:
            continue
        if not all(
            value and len(value.encode("utf-8")) <= 255 and not re.search(r"[\r\n\x00]", value)
            for value in (username, password)
        ):
            parser.error(f"provide both valid MOVIEGO_TEST_{role} credential fields")
        accounts[role] = (username, password, code)

    def connect():
        return socket.create_connection((args.host, args.port), timeout=args.timeout)

    @contextmanager
    def signed_in(role="USER"):
        if role not in accounts:
            raise SkipTest(f"set MOVIEGO_TEST_{role} and MOVIEGO_TEST_{role}_PASSWORD")
        username, password, code = accounts[role]
        with connect() as peer:
            request(peer, code, "LOGIN", username, password)
            try:
                yield peer
            except BaseException:
                # Release the account when the stream is still usable.
                try:
                    request(peer, "1030", "LOGOUT")
                except (TestFailure, OSError):
                    pass
                raise
            else:
                request(peer, "1030", "LOGOUT")

    def tcp_connection():
        with connect():
            pass

    def guest_request(expected, *lines):
        with connect() as peer:
            request(peer, expected, *lines)

    def login_valid():
        with signed_in():
            pass

    def logout():
        username, password, code = accounts["USER"]
        with connect() as peer:
            request(peer, code, "LOGIN", username, password)
            request(peer, "1030", "LOGOUT")
            request(peer, "3028", "VIEW_TICKETS")

    def exit_connection():
        with connect() as peer:
            request(peer, "1030", "EXIT")
            if peer.recv(1) != b"":
                raise TestFailure("expected EOF after EXIT")

    def user_request(expected, *lines, **options):
        with signed_in() as peer:
            request(peer, expected, *lines, **options)

    def view_tickets():
        with signed_in() as peer:
            peer.sendall(frame("VIEW_TICKETS"))
            code, lines = recv_message(peer)
            if code == "3013" and not lines:
                return
            if (
                code != "1080"
                or len(lines) < 3
                or not lines[0].startswith("ID ")
                or lines[-1] != f"You has total: {len(lines) - 2} ticket(s)"
            ):
                raise TestFailure(f"expected ticket table (1080) or no tickets (3013), got {code}")

    def pipelined_requests():
        with signed_in() as peer:
            peer.sendall(frame("SHOW_FILMS") + frame("SHOW_ALL_USERS"))
            expect(recv_message(peer), "1073", "3008", r"[0-9]+\. .*")
            expect(recv_message(peer), "3029")

    def manager_rooms():
        with signed_in("MANAGER") as peer:
            cinemas = request(peer, "1071", "SHOW_CINEMAS", empty="3004", pattern=r"[0-9]+\. .*")
            if not cinemas:
                raise SkipTest("requires at least one cinema")
            cinema_id = cinemas[0].split(".", 1)[0]
            request(peer, "1080", "SHOW_ROOMS_BY_CINEMA", cinema_id, empty="3012",
                    pattern=r"ID: [0-9]+ \| .* \| [0-9]+ seats")

    def admin_users():
        with signed_in("ADMIN") as peer:
            request(peer, "1300", "SHOW_ALL_USERS", pattern=r"[0-9]+\|.*\|.*\|(Admin|Manager|User|Unknown)")

    tests = [
        ("TCP connection", tcp_connection),
        ("Login valid", login_valid),
        ("Login invalid", lambda: guest_request("2011", "LOGIN", f"regression-{uuid4().hex}", "invalid")),
        ("Logout", logout),
        ("View films", lambda: user_request("1073", "SHOW_FILMS", empty="3008", pattern=r"[0-9]+\. .*")),
        ("View showtimes", lambda: user_request("1072", "SHOW_PREMIERED_TIME", empty="3006",
                                              pattern=r"[0-9]{2}:[0-9]{2} - [0-9]{2}:[0-9]{2}")),
        ("View tickets", view_tickets),
        ("View categories", lambda: user_request("1065", "SHOW_CATEGORIES", empty="3002", pattern=r"[0-9]+\. .*")),
        ("Invalid ticket ID", lambda: user_request("3018", "VIEW_TICKET_DETAIL", "0")),
        ("Unknown command", lambda: guest_request("3018", "REGRESSION_UNKNOWN")),
        ("Missing argument", lambda: guest_request("3018", "LOGIN", "username")),
        ("Extra argument", lambda: user_request("3018", "SHOW_FILMS", "extra")),
        ("Pipelined requests", pipelined_requests),
        ("Guest denied", lambda: guest_request("3028", "VIEW_TICKETS")),
        ("User denied Admin command", lambda: user_request("3029", "SHOW_ALL_USERS")),
        ("User denied Manager command", lambda: user_request("3029", "SHOW_ROOMS_BY_CINEMA", "1")),
        ("Manager view rooms", manager_rooms),
        ("Admin view users", admin_users),
        ("Exit", exit_connection),
    ]
    counts = dict.fromkeys(("Passed", "Failed", "Skipped"), 0)
    for name, function in tests:
        counts[run_test(name, function)] += 1
    for label, count in counts.items():
        print(f"{label}: {count}")
    print(f"Total: {len(tests)}")
    return int(counts["Failed"] > 0)


if __name__ == "__main__":
    raise SystemExit(main())
