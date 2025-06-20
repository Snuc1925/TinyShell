@echo off
echo ==== Bat script đang chạy... ====

echo Bước 1: Tạo thư mục Demo
mkdir Demo

echo Bước 2: Chuyển vào thư mục Demo
cd Demo

echo Bước 3: Tạo file hello.txt
echo Hello, world! > hello.txt

echo Bước 4: Hiển thị nội dung file
type hello.txt

echo Bước 5: Đợi người dùng nhấn phím
pause

echo Bước 6: Quay về thư mục gốc
cd ..

echo Bước 7: Xoá thư mục Demo và toàn bộ nội dung bên trong
rmdir /S /Q Demo

echo === Đã hoàn tất! ===
pause
