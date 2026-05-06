import re

with open('mainwindow.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the static helper function and replace the call with inline code
old = (
    '    rebuildExtractionCompleter(extractionTable, extractionCompleterModel);\n'
    '}\n'
    'static void rebuildExtractionCompleter(QTableWidget *table, QStringListModel *model)\n'
    '{\n'
    '    if (!table || !model) return;\n'
    '    QSet<QString> seen;\n'
    '    QStringList suggestions;\n'
    '    for (int r = 0; r < table->rowCount(); ++r) {\n'
    '        for (int c = 0; c < table->columnCount(); ++c) {\n'
    '            QTableWidgetItem *it = table->item(r, c);\n'
    '            if (!it) continue;\n'
    '            const QString val = it->text().trimmed();\n'
    '            if (!val.isEmpty() && !seen.contains(val)) {\n'
    '                seen.insert(val);\n'
    '                suggestions << val;\n'
    '            }\n'
    '        }\n'
    '    }\n'
    '    suggestions.sort(Qt::CaseInsensitive);\n'
    '    model->setStringList(suggestions);\n'
    '}'
)

new = (
    '    // Rebuild completer suggestions\n'
    '    if (extractionCompleterModel) {\n'
    '        QSet<QString> seen;\n'
    '        QStringList suggestions;\n'
    '        for (int r = 0; r < extractionTable->rowCount(); ++r) {\n'
    '            for (int c = 0; c < extractionTable->columnCount(); ++c) {\n'
    '                QTableWidgetItem *it = extractionTable->item(r, c);\n'
    '                if (!it) continue;\n'
    '                const QString val = it->text().trimmed();\n'
    '                if (!val.isEmpty() && !seen.contains(val)) {\n'
    '                    seen.insert(val);\n'
    '                    suggestions << val;\n'
    '                }\n'
    '            }\n'
    '        }\n'
    '        suggestions.sort(Qt::CaseInsensitive);\n'
    '        extractionCompleterModel->setStringList(suggestions);\n'
    '    }\n'
    '}'
)

if old in content:
    content = content.replace(old, new)
    with open('mainwindow.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print('REPLACED OK')
else:
    print('NOT FOUND - checking nearby...')
    idx = content.find('rebuildExtractionCompleter')
    if idx >= 0:
        print(repr(content[idx-5:idx+200]))
