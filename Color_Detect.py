import cv2
import numpy as np
import serial
import time

# ===== SERIAL =====
ser = serial.Serial('COM8', 115200, timeout=0.1)   # đổi COM cho đúng
time.sleep(2)

# ===== CAMERA =====
cap = cv2.VideoCapture(1)

detecting = False          # chỉ nhận diện khi có lệnh begin
serial_buffer = ""         # buffer nhận chuỗi từ serial

while True:
    # ===== ĐỌC SERIAL ĐỂ CHỜ LỆNH begin =====
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting).decode(errors='ignore')
        serial_buffer += data

        if "begin" in serial_buffer:
            print("Nhan duoc lenh begin -> bat dau nhan dien")
            detecting = True
            serial_buffer = ""   # xóa buffer sau khi nhận lệnh

    # Luôn đọc camera để hiển thị hình
    ret, frame = cap.read()
    if not ret:
        print("Không lấy được hình")
        break

    # Nếu chưa có lệnh begin thì chỉ hiện camera
    if not detecting:
        cv2.imshow("Original", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        continue

    # ===== XỬ LÝ ẢNH KHI ĐANG detect =====
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # ===== TÁCH MÀU ĐỎ =====
    lower_red1 = np.array([0, 120, 70])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([170, 120, 70])
    upper_red2 = np.array([180, 255, 255])

    mask_red1 = cv2.inRange(hsv, lower_red1, upper_red1)
    mask_red2 = cv2.inRange(hsv, lower_red2, upper_red2)
    mask_red = mask_red1 | mask_red2
    red_result = cv2.bitwise_and(frame, frame, mask=mask_red)

    # ===== TÁCH MÀU XANH LÁ =====
    lower_green = np.array([35, 80, 80])
    upper_green = np.array([85, 255, 255])

    mask_green = cv2.inRange(hsv, lower_green, upper_green)
    green_result = cv2.bitwise_and(frame, frame, mask=mask_green)

    # ===== TÍNH DIỆN TÍCH =====
    red_area = cv2.countNonZero(mask_red)
    green_area = cv2.countNonZero(mask_green)

    print("Red area:", red_area, "| Green area:", green_area)

    # ===== GỬI KẾT QUẢ RỒI DỪNG NHẬN DIỆN =====
    if red_area > 100000:
        ser.write(b'r')
        print("Da gui: r")
        detecting = False

    elif green_area > 100000:
        ser.write(b'g')
        print("Da gui: g")
        detecting = False

    # ===== HIỂN THỊ =====
    cv2.imshow("Original", frame)
    cv2.imshow("Red Color", red_result)
    cv2.imshow("Green Color", green_result)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
ser.close()
cv2.destroyAllWindows()