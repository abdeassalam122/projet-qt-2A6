$f = 'mainwindow.cpp'
$content = [System.IO.File]::ReadAllText($f, [System.Text.Encoding]::UTF8)

# Find the orphaned DDL block and remove it
$startMarker = "`n        return true;`n    };`n`n    QString lastError;"
$endMarker = "`n    return true;`n}`n`nvoid MainWindow::loadClientsFromOracle()"

$startIdx = $content.IndexOf($startMarker)
$endIdx   = $content.IndexOf($endMarker)

if ($startIdx -ge 0 -and $endIdx -ge 0) {
    $before = $content.Substring(0, $startIdx)
    $after  = $content.Substring($endIdx)
    $content = $before + $after
    [System.IO.File]::WriteAllText($f, $content, [System.Text.Encoding]::UTF8)
    Write-Host "Removed orphaned DDL block OK"
} else {
    Write-Host "Markers not found: start=$startIdx end=$endIdx"
    # Show context
    $idx = $content.IndexOf("return true;`n    };")
    Write-Host "Orphan at: $idx"
    if ($idx -ge 0) { Write-Host $content.Substring($idx, 100) }
}
