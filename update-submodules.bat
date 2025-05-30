@echo off
SETLOCAL

:: Check if a commit message argument is provided
IF "%~1"=="" (
    echo Error: Please provide a commit message as an argument.
    echo Usage: update_submodules.bat "Your commit message"
    exit /b 1
)

SET COMMIT_MESSAGE=%~1

echo.
echo --- Checking out main branch and pulling latest changes ---
git checkout main
git pull origin main

echo --- Updating submodules (aquarium-iot-shared-lib) ---
git submodule update --remote --merge --recursive

echo.
echo === Staging updated submodule references ===
git add .

echo.
echo === Committing submodule updates ===
git commit -m "Update submodules: %COMMIT_MESSAGE%"

echo.
echo === Pushing to remote ===
git push origin main

echo.
echo ✅ Submodule updated and committed.
ENDLOCAL
