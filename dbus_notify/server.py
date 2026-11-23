import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

BUS_NAME = "org.example.Lab2"
OBJECT_PATH = "/org/example/Lab2"
INTERFACE = "org.example.Lab2.Notification"


class NotificationServer(dbus.service.Object):
    def __init__(self, name, loop):
        super().__init__(name, OBJECT_PATH)
        self.loop = loop

    @dbus.service.method(INTERFACE, in_signature="ss", out_signature="s")
    def Notify(self, title, message):
        print(f"[알림 수신] 제목: {title}", flush=True)
        print(f"[알림 내용] {message}", flush=True)
        return "서버가 알림을 받았습니다."

    @dbus.service.method(INTERFACE, in_signature="", out_signature="s")
    def Quit(self):
        print("[서버] 종료 요청 수신", flush=True)
        GLib.timeout_add(200, self.loop.quit)
        return "서버 종료 요청을 받았습니다."


def main():
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    name = dbus.service.BusName(BUS_NAME, bus=bus, do_not_queue=True)
    loop = GLib.MainLoop()
    server = NotificationServer(name, loop)

    print("[서버] D-Bus 알림 수신 대기 중", flush=True)
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    finally:
        server.remove_from_connection()
        print("[서버] 종료 완료", flush=True)


if __name__ == "__main__":
    main()
