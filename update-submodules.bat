@echo off
SETLOCAL

echo.
echo === Updating submodule: aquarium-iot-shared-lib ===
cd subprojects\aquarium-iot-shared-lib

echo --- Checking out main branch and pulling latest changes ---
git checkout main
git pull origin main

cd ..\..

echo.
echo === Staging updated submodule references ===
git add subprojects\aquarium-iot-shared-lib

echo.
echo === Committing submodule updates ===
git commit -m "Update submodules: aquarium-iot-shared-lib to latest main"

echo.
echo === Pushing to remote ===
git push origin main

echo.
echo ✅ All submodules updated and committed.
ENDLOCAL
