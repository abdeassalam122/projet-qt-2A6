$files = @{
    'reception.cpp' = @('AWSS\.RECEPTION', 'RECEPTION')
    'client.cpp'    = @('AWSS\.CLIENT',    'CLIENT')
    'citerne.cpp'   = @('AWSS\.CITERNE',   'CITERNE')
    'extraction.cpp'= @('AWSS\.EXTRACTION','EXTRACTION')
}

foreach ($f in $files.Keys) {
    $content = [System.IO.File]::ReadAllText($f, [System.Text.Encoding]::UTF8)
    $old = $files[$f][0]
    $new = $files[$f][1]
    $content = $content -replace $old, $new
    # Also fix SEQ in extraction
    if ($f -eq 'extraction.cpp') {
        $content = $content -replace 'AWSS\.SEQ_EXTRACTION', 'SEQ_EXTRACTION'
    }
    [System.IO.File]::WriteAllText($f, $content, [System.Text.Encoding]::UTF8)
    Write-Host "$f done"
}
