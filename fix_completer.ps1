$file = 'mainwindow.cpp'
$content = [System.IO.File]::ReadAllText($file, [System.Text.Encoding]::UTF8)

$idx = $content.IndexOf('rebuildExtractionCompleter(extractionTable')
if ($idx -lt 0) { Write-Host "call not found"; exit }

# Find the end of the static function (closing brace after model->setStringList)
$endMarker = 'model->setStringList(suggestions);' + "`r`n" + '}'
$endIdx = $content.IndexOf($endMarker, $idx)
if ($endIdx -lt 0) {
    $endMarker = 'model->setStringList(suggestions);' + "`n" + '}'
    $endIdx = $content.IndexOf($endMarker, $idx)
}
if ($endIdx -lt 0) { Write-Host "end marker not found"; exit }

$endIdx += $endMarker.Length

$before = $content.Substring(0, $idx)
$after  = $content.Substring($endIdx)

$inline = @'
    // Rebuild completer suggestions
    if (extractionCompleterModel) {
        QSet<QString> seen;
        QStringList suggestions;
        for (int r = 0; r < extractionTable->rowCount(); ++r) {
            for (int c = 0; c < extractionTable->columnCount(); ++c) {
                QTableWidgetItem *it = extractionTable->item(r, c);
                if (!it) continue;
                const QString val = it->text().trimmed();
                if (!val.isEmpty() && !seen.contains(val)) {
                    seen.insert(val);
                    suggestions << val;
                }
            }
        }
        suggestions.sort(Qt::CaseInsensitive);
        extractionCompleterModel->setStringList(suggestions);
    }
}
'@

$newContent = $before + $inline + $after
[System.IO.File]::WriteAllText($file, $newContent, [System.Text.Encoding]::UTF8)
Write-Host "Done. Replaced at index $idx"
