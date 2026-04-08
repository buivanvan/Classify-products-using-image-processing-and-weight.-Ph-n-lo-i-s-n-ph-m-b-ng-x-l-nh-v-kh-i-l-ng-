# Classify-products-using-image-processing-and-weight.
PHÂN LOẠI SẢN PHẨM BẰNG XỬ LÝ ẢNH VÀ KHỐI LƯỢNG
Sử dụng OpenCV để nhận dạng màu sắc và gửi kết quả xuống ESP32 thông qua UART. Sau khi nhận được dữ liệu màu sắc của vật phẩm đồng thời kết hợp với khối lượng để phân loại sản phẩm. Sản phẩm được đẩy ra băng tải và được tự động phân loại, đếm số lượng bằng RC Servo, cảm biến hồng ngoại.
- Gồm 3 file
+ File source code ESP32 đọc dữ liệu từ HX711, IR Sensor, điều khiển RC Servo
+ File source code Python xử lý ảnh bằng OpenCV để nhận diện màu sắc
+ File .AIA của MIT App Inventor để tạo app trên điện thoại nhằm xem tổng số lượng sản phẩm đã phân loại và số lượng từng loại

Use OpenCV to recognize colors and send the results to the ESP32 via UART. After receiving the color data of the object, it is combined with the weight to classify the product. The products are pushed onto a conveyor belt and automatically sorted, while counting is performed using an RC servo and an infrared sensor.

* Includes 3 files:
- ESP32 source code for reading data from HX711 and IR sensor, and controlling the RC servo
- Python source code for image processing using OpenCV to recognize colors
- MIT App Inventor (.AIA) file to create a mobile app for monitoring the total number of classified products and the quantity of each type
