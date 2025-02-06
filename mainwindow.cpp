#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
#include <iostream>
int32_t MainWindow::fileContainsText(QList<QString>& lines, QString toFind)
{
    for (int i=0; i<lines.size(); ++i) {
        int idx = lines.at(i).indexOf(toFind);
        if (idx != -1) {
            std::cout<<"found "<<toFind.toStdString()<<" at lineNo="<<i<<'\n';
            return i;
        }
    }
    
    return -1;
}

void MainWindow::insertMultipleLines(QList<QString> &lines, int32_t k, QStringList toAdd) {
    for (QString text : toAdd) {
        text.append("\r\n");
        lines.insert(k++, text);
    }
}

void MainWindow::writeLinesToFile(QList<QString> lines, QFile& langFile)
{
    langFile.open(QIODevice::WriteOnly);
    for (int i=0; i<lines.size(); ++i) {
        langFile.write(lines.at(i).toUtf8());
    }
    langFile.close();
}

int32_t MainWindow::processLangFolder(QDir& directory)
{
    int32_t changes = 0;
    
    QDir langFolder(directory);
    std::cout<<langFolder.exists()<<'\n';
    langFolder.cd("shaders/lang");
    QString lang = langFolder.filePath("en_US.lang");
    QFile langFile(lang);
    std::cout<<langFile.fileName().toStdString()<<'\n';
    QList<QString> lines = getAllLinesFromFile(langFile);
    if (fileContainsText(lines, "option.FORCE_PBR_PLUS") == -1) {
        if (int32_t idx = fileContainsText(lines, "value.RP_MODE.3"); idx != -1) {
            ++changes;
            this->insertMultipleLines(lines, idx + 2,
                                      {"option.FORCE_PBR_PLUS=Integrated PBR+",
                                       "option.FORCE_PBR_PLUS.comment=Enable PBR+ even when using SEUS/LabPBR",
                                       "value.FORCE_PBR_PLUS.0=Off",
                                       "value.FORCE_PBR_PLUS.1=On",
                                       ""});
        }
    }
    QFile::copy(langFile.fileName(), langFolder.path() + "/" + "en_US.lang.bak");
    writeLinesToFile(lines, langFile);
    
    return changes;
}

int32_t MainWindow::processLibFolder(QDir& directory)
{
    int32_t changes = 0;
    
    QDir libFolder(directory);
    libFolder.cd("shaders/lib");
    QString common = libFolder.filePath("common.glsl");
    QFile commonFile(common);
    QList<QString> lines = getAllLinesFromFile(commonFile);
    if (fileContainsText(lines, "#define RP_MODE 0") == -1) {
        if (int32_t idx = fileContainsText(lines, "#define RP_MODE 1"); idx != -1) {
            ++changes;
            
            lines.remove(idx);
            this->insertMultipleLines(lines, idx,
                                      {"\t#define RP_MODE 0 //[0 3 2]",
                                       "\t",
                                       "\t#define FORCE_PBR_PLUS 1 //[1 0]"});
        }
    }
    if (fileContainsText(lines, "#if FORCE_PBR_PLUS == 1") == -1) {
        if (int32_t idx = fileContainsText(lines, "#if SHADER_STYLE == 1"); idx != -1) {
            ++changes;
            
            this->insertMultipleLines(lines, idx - 1,
                                      {"\t",
                                       "\t#if FORCE_PBR_PLUS == 1",
                                       "\t\t#define IPBR",
                                       "\t\t#define IPBR_OVERRIDE",
                                       "\t\t//#define GENERATED_NORMALS",
                                       "\t\t//#define COATED_TEXTURES",
                                       "\t\t//#define FANCY_GLASS",
                                       "\t\t//#define GREEN_SCREEN_LIME",
                                       "\t#endif"});
        }
    }
    QFile::copy(commonFile.fileName(), libFolder.path() + "/" + "common.glsl.bak");
    writeLinesToFile(lines, commonFile);
    
    return changes;
}

int32_t MainWindow::processPropertiesFile(QDir& directory)
{
    int32_t changes = 0;
    
    QDir shadersFolder(directory);
    shadersFolder.cd("shaders");
    QString props = shadersFolder.filePath("shaders.properties");
    QFile propsFile(props);
    QList<QString> lines = getAllLinesFromFile(propsFile);
    if (fileContainsText(lines, "FORCE_PBR_PLUS") == -1) {
        if (int32_t idx = fileContainsText(lines, "Screen Setup"); idx != -1) {
            ++changes;
            
            QString currentLine = lines.at(idx + 1);
            currentLine.remove("\r");
            currentLine.remove("\n");
            lines.remove(idx + 1);
            lines.insert(idx + 1, currentLine + " FORCE_PBR_PLUS\r\n");
        }
    }
    QFile::copy(propsFile.fileName(), shadersFolder.path() + "/" + "shaders.properties.bak");
    writeLinesToFile(lines, propsFile);
    
    return changes;
}

QList<QString> MainWindow::getAllLinesFromFile(QFile& propsFile)
{
    propsFile.open(QIODevice::ReadOnly);
    QList<QString> lines;
    while (1) {
        QByteArray bytes = propsFile.readLine();
        if (bytes.isNull())
            break;
        lines.append(QString(bytes));
    }
    propsFile.close();
    
    return lines;
}

int32_t MainWindow::processProgramFolder(QDir& directory)
{
    int32_t changes = 0;
    
    QDir programFolder(directory);
    programFolder.cd("shaders/program");
    QStringList fileList = programFolder.entryList();
    
    for (QString file : fileList) {
        if (file == "." || file == ".." || file.endsWith(".bak")) {
            continue;
        }
        
        QString programFilePath = programFolder.filePath(file);
        QFile programFile(programFilePath);
        QList<QString> lines = getAllLinesFromFile(programFile);
        
        int32_t lineNo = 0;
        int32_t ipbrStart = -1;
        int32_t ipbrElse = -1;
        int32_t ipbrEndIf = -1;
        QString initialIpbrCondition;
        bool modifiedLines = false;
        int32_t ifNestingLevel = 0;
        int32_t ipbrIfNestingLevel = -1;
        for (QString line : lines) {
            if (line.trimmed().startsWith("#ifdef") || line.trimmed().startsWith("#if")) {
                ++ifNestingLevel;
            }
            
            if (line.trimmed().startsWith("#ifdef") && line.contains("IPBR")) {
                initialIpbrCondition = line;
                ipbrStart = lineNo;
                ipbrIfNestingLevel = ifNestingLevel;
            } else if (line.trimmed().startsWith("#else")) {
                if (ipbrStart != -1 && ifNestingLevel == ipbrIfNestingLevel) {
                    ipbrElse = lineNo;
                }
            } else if (line.trimmed().startsWith("#endif")) {
                if (ipbrStart != -1 && ipbrElse != -1 && ifNestingLevel == ipbrIfNestingLevel) {
                    ipbrEndIf = lineNo;
                    
                    int32_t ifStartLine = ipbrStart + 1;
                    int32_t ifEndLine = ipbrElse - 1;
                    int32_t ifLinecount = ifEndLine - ifStartLine + 1;
                    
                    int32_t elseStartLine = ipbrElse + 1;
                    int32_t elseEndLine = ipbrEndIf - 1;
                    int32_t elseLinecount = elseEndLine - elseStartLine + 1;
                    
                    QStringList ipbrLines = lines.sliced(ifStartLine, ifLinecount);
                    QStringList elseLines = lines.sliced(elseStartLine, elseLinecount);
                    
                    int32_t linesToRemove = ipbrEndIf - ipbrStart + 1;
                    
                    lines.remove(ipbrStart, linesToRemove);
                    lines.insert(ipbrStart, "#if defined IPBR_OVERRIDE || !defined IPBR\r\n");
                    int32_t k = 1;
                    for (QString subLine : elseLines) {
                        lines.insert(ipbrStart + (k++), subLine);
                    }
                    lines.insert(ipbrStart + (k++), "#endif\r\n");
                    lines.insert(ipbrStart + (k++), "\r\n");
                    lines.insert(ipbrStart + (k++), "#ifdef IPBR\r\n");
                    for (QString subLine : ipbrLines) {
                        lines.insert(ipbrStart + (k++), subLine);
                    }
                    lines.insert(ipbrStart + (k++), "#endif\r\n");
                    
                    ipbrStart = -1;
                    ipbrElse = -1;
                    ipbrEndIf = -1;
                    
                    modifiedLines = true;
                    ++changes;
                } else {
                    if (ifNestingLevel == ipbrIfNestingLevel) {
                        ipbrStart = -1;
                        ipbrElse = -1;
                    }
                }
                
                --ifNestingLevel;
            }
            
            ++lineNo;
        }
        
        if (modifiedLines) {
            std::cout<<"Modified: "<<programFile.fileName().toStdString()<<'\n';
            QFile::copy(programFile.fileName(), programFolder.path() + "/" + QFileInfo(programFile).fileName() + ".bak");
            writeLinesToFile(lines, programFile);
        }
    }
    
    return changes;
}

void MainWindow::on_pushButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    QDir directory(dir);
    if (directory.exists()) {
        this->ui->langEdit->setText(QString::number(processLangFolder(directory)));
        this->ui->commonEdit->setText(QString::number(processLibFolder(directory)));
        this->ui->propertiesEdit->setText(QString::number(processPropertiesFile(directory)));
        this->ui->programEdit->setText(QString::number(processProgramFolder(directory)));
    }
}

