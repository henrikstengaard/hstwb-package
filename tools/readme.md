List files with carrier return and new line, used to ensure AmigaDOS scripts only contain newline:
```
Get-ChildItem .\hstwb\ -Recurse  | where { ! $_.PSIsContainer } | Foreach-Object { $text = Get-Content $_.FullName -Raw; if ($text -match "\r\n") { Write-Host $_.FullName } }
```