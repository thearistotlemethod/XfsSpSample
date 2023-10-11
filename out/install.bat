@echo off
goto check_Permissions

:check_Permissions
    net session >nul 2>&1
    if %errorLevel% == 0 (
		regedit.exe /S software.reg

		xcopy "SampleSP.dll" "%windir%\system32\"  /s /h
    ) else (
		echo Administrative permissions required.
        echo Failure: Current permissions inadequate.
    )
