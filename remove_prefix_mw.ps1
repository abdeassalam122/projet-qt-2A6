$f = 'mainwindow.cpp'
$content = [System.IO.File]::ReadAllText($f, [System.Text.Encoding]::UTF8)
$content = $content -replace 'AWSS\.', ''
[System.IO.File]::WriteAllText($f, $content, [System.Text.Encoding]::UTF8)
Write-Host "mainwindow.cpp done - removed all AWSS. prefixes"
