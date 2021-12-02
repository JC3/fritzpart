/*----------------------------------------------------------------------
Fritzpart - Generates Fritzing parts from a part description script.
Copyright (C) 2021, Jason Cipriani <jason.cipriani.dev@gmail.com>
Not affiliated with Fritzing.

This file is part of Fritzpart.

Fritzpart is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

https://github.com/JC3/fritzpart
----------------------------------------------------------------------*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// i feel like qt probably has something with this behavior built-in already but whatever.
// just a string map but unlike QMap::value(), also provides a way to substitute defaults
// for values that are present but are empty strings.
class PropertyMap : public QMap<QString,QString> {
public:
    QString getValue (const QString &key, const QString &defaultValue = QString()) const {
        QString v = value(key, defaultValue);
        return v.isEmpty() ? defaultValue : v;
    }
};

struct Pin {
    double x;
    double y;
    QString name;
    bool square;
    double hole;
    double ring;
    int number;
    Pin () : x(0), y(0), square(false), hole(0.9), ring(0.508), number(-1) { }
    // temporary parsing context stuff
    bool origleft;
    bool origtop;
};

struct Part {
    QString units;  // todo: define a set of units instead of allowing free text. mm, in, micron, mil, thou probably
    double width;
    double height;
    double outline; // todo: different default depending on units
    QList<Pin> pins;
    QString color;
    double corner;
    QString schematic;
    QString schematicmod;
    int mingrid[2];
    int extragrid[2];
    QString bbtext;
    QString bbtextcolor;
    double bbtextsize; // todo: different default depending on units
    QString sctext;
    PropertyMap metadata;
    PropertyMap metaprops;
    QStringList metatags;
    QString filename;
    Part () : units("mm"), width(0), height(0), outline(0.254), color("#116b9e"), corner(0), schematic("edge"),
        mingrid{0,0}, extragrid{0,0}, bbtext("$partnumber"), bbtextcolor("#ffffff"), bbtextsize(5.08), sctext("$title"),
        metatags({"fritzpart"}) { }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void loadFile (QString filename);
    void saveFile (QString filename);
private slots:
    void on_actOpenFile_triggered();
    void on_actSaveFile_triggered();
    void on_actCompile_triggered();

private:
    Ui::MainWindow *ui;
    QSettings settings;
    void saveBasicPart (const Part &part, QString prefix);
};

#endif // MAINWINDOW_H
