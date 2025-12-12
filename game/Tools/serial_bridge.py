import socket
import sys
import threading
import time

import serial
import serial.tools.list_ports


HOST = "127.0.0.1"
PORT = 24072


def main():
    print("::::::::: :::    ::: :::::::: :::    ::::::    ::::::    :::::::    :::::::::::::: \n"\
          ":+:    :+::+:    :+::+:    :+::+:   :+: :+:    :+::+:    :+::+:+:   :+:    :+:     \n"\
          "+:+    +:++:+    +:++:+       +:+  +:+  +:+    +:++:+    +:+:+:+:+  +:+    +:+     \n"\
          "+#+    +:++#+    +:++#+       +#++:++   +#++:++#+++#+    +:++#+ +:+ +#+    +#+     \n"\
          "+#+    +#++#+    +#++#+       +#+  +#+  +#+    +#++#+    +#++#+  +#+#+#    +#+     \n"\
          "#+#    #+##+#    #+##+#    #+##+#   #+# #+#    #+##+#    #+##+#   #+#+#    #+#     \n"\
          "#########  ########  ######## ###    ######    ### ######## ###    ####    ###\n"\
          "                                 Serial Bridge\n"\
          "                                 ver. production")
    
    print()
    
    port_name = get_serial_port()
    baud_rate = get_baud_rate()

    print()

    print(f"[serial] selected port: {port_name}")
    print(f"[serial] baud rate: {baud_rate}")

    try:
        srl = serial.Serial(port_name, baudrate=baud_rate, timeout=0.01)
    except serial.SerialException as e:
        print(f"failed to open serial port: {e}")
        return 1

    print(f"[serial] open port: {srl.portstr}")

    print()

    # TCP
    server_sck = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sck.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sck.bind((HOST, PORT))
    server_sck.listen(1)

    print(f"[socket] waiting for Unity client connection...({HOST}:{PORT})")

    try:
        client_sck, addr = server_sck.accept()
    except KeyboardInterrupt:
        print("terminated by user.")
        srl.close()
        server_sck.close()
        return 0

    print(f"[socket] client connection successful: {addr}")

    running_flag = {"running": True}

    srl2sck_t = threading.Thread(target=serial_to_socket, args=(srl, client_sck, running_flag), daemon=True)
    sck2srl_t = threading.Thread(target=socket_to_serial, args=(client_sck, srl, running_flag), daemon=True)

    srl2sck_t.start()
    sck2srl_t.start()

    print("bridge in progress(press ctrl + c to exit)")

    try:
        while running_flag["running"]:
            time.sleep(0.1)
    finally:
        running_flag["running"] = False
        time.sleep(0.1)

        try:
            client_sck.close()
        except Exception:
            pass

        try:
            server_sck.close()
        except Exception:
            pass

        try:
            srl.close()
        except Exception:
            pass

    print("terminated.")
    return 0


def get_serial_port():
    ports = list(serial.tools.list_ports.comports())

    print("=== available serial ports ===")
    if not ports:
        print("no ports were automatically detected.")
        print("please enter the port name manually.")
    else:
        for idx, p in enumerate(ports):
            print(f"[{idx}] {p.device}  ({p.description})")
    print()

    while True:
        if ports:
            user_input = input("enter the index number of the port to use, or enter the port name directly(e.g., (win)COM3, (mac)/dev/cu.usbmodem1101): ").strip()
        else:
            user_input = input("enter the port name(e.g., (win)COM3, (mac)/dev/cu.usbmodem1101): ").strip()

        if not user_input:
            print("the field is empty.")
            continue

        if user_input.isdigit() and ports:
            idx = int(user_input)
            if 0 <= idx < len(ports):
                return ports[idx].device
            else:
                print("invalid index number.")
        else:
            return user_input


def get_baud_rate():
    while True:
        user_input = input("enter the baud rate(default: 115200)").strip()
        if not user_input:
            return 115200
        
        try:
            baud = int(user_input)
            if baud <= 0:
                raise ValueError()
            return baud
        except ValueError:
            print("invalid value.")


def serial_to_socket(srl, client_sck, running_flag):
    try:
        while running_flag["running"]:
            try:
                data = srl.read(srl.in_waiting or 1)
                if data:
                    client_sck.sendall(data)
            except (serial.SerialException, OSError) as e:
                print(f"[serial to socket] serial error: {e}")
                running_flag["running"] = False
                break
            except (BrokenPipeError, ConnectionResetError) as e:
                print(f"[serial to socket] socket error: {e}")
                running_flag["running"] = False
                break

            # CPU 점유율 폭증 방지
            time.sleep(0.001)
    finally:
        print("[serial to socket] terminated")


def socket_to_serial(client_sck, srl, running_flag):
    try:
        while running_flag["running"]:
            try:
                data = client_sck.recv(1024)
                if not data:
                    running_flag["running"] = False
                    break
                srl.write(data)
            except (BrokenPipeError, ConnectionResetError, OSError) as e:
                print(f"[socket to serial] socket error: {e}")
                running_flag["running"] = False
                break
            except serial.SerialException as e:
                print(f"[socket to serial] serial error: {e}")
                running_flag["running"] = False
                break

            time.sleep(0.001)
    finally:
        print("[socket to serial] terminated")


if __name__ == "__main__":
    sys.exit(main())
