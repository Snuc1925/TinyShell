import time
import signal
import sys

def signal_handler(sig, frame):
    print("\nPython process received signal", sig, file=sys.stderr)
    sys.stderr.flush()
    time.sleep(1)  # Đảm bảo thông báo được in ra trước khi tạm dừng

signal.signal(signal.SIGINT, signal_handler)

def main():
    print("Python counter started (PID: {})".format(os.getpid()))
    try:
        for i in range(1, 30):
            print(i)
            time.sleep(0.1)  # In mỗi số cách nhau 0.1 giây
    except KeyboardInterrupt:
        print("\nKeyboardInterrupt received in Python", file=sys.stderr)
        sys.stderr.flush()

if __name__ == "__main__":
    import os
    main()