@echo off
echo ==== Bat script is running... ====

echo Step 1: Create Demo folder
mkdir Demo

echo Step 2: Move to Demo
cd Demo

echo Step 3: Create file hello.txt
echo Hello, world! > hello.txt

echo Step 4: Show file's text
type hello.txt

echo Step 5: Wait for key presses
pause

echo Step 6: Return to the original folder
cd ..

echo Step 7: Delete Demo folder and its files
rmdir /S /Q Demo

echo === Done! ===
pause
