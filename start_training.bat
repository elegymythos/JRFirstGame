@echo off

echo ==================================================
echo   JRFirstGame RL Training - One-click Start
echo ==================================================
echo.

cd /d "%~dp0"

if not exist "rl_env\Scripts\activate.bat" (
    echo [ERROR] Virtual environment 'rl_env' not found.
    echo Please create it manually:
    echo   python -m venv rl_env
    echo   rl_env\Scripts\activate
    echo   pip install -r rl\requirements.txt
    echo.
    pause
    exit /b 1
)

echo [INFO] Activating virtual environment...
call "rl_env\Scripts\activate.bat"
if errorlevel 1 (
    echo [ERROR] Activation failed.
    pause
    exit /b 1
)

python -c "import gradio" 2>nul
if errorlevel 1 (
    echo [WARN] gradio not found, installing...
    pip install gradio matplotlib
    if errorlevel 1 (
        echo [ERROR] Installation failed. Run manually: pip install gradio matplotlib
        pause
        exit /b 1
    )
)

echo.
echo [INFO] Starting WebUI...
echo [INFO] Opening http://localhost:7860 in browser...
echo [INFO] Press Ctrl+C to stop.
echo.

start "" "http://localhost:7860"

python rl/webui.py

echo.
echo [INFO] WebUI stopped.
pause