Set WinScriptHost = CreateObject("WScript.Shell")
WinScriptHost.Run Chr(34) & "C:\Program Files (x86)\MetaTrader 4\terminal.exe" & Chr(34) & " genstock.txt" , 6, true
