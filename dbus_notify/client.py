import sys
import dbus

BUS_NAME = "org.example.Lab2"
OBJECT_PATH = "/org/example/Lab2"
INTERFACE = "org.example.Lab2.Notification"


def main():
    try:
        bus = dbus.SessionBus()
        proxy = bus.get_object(BUS_NAME, OBJECT_PATH)
        service = dbus.Interface(proxy, INTERFACE)

        if sys.argv[1:] == ["--quit"]:
            reply = service.Quit()
        else:
            message = " ".join(sys.argv[1:]) or "Hello D-Bus"
            print(f"[클라이언트] 알림 전송: {message}")
            reply = service.Notify("Lab2", message)

        print(f"[서버 응답] {reply}")
        return 0
    except dbus.DBusException as error:
        print(f"D-Bus 오류: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
