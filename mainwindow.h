#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include <QTextDocument>

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
    bool bbpinlabels;
    QString bbpinlabelcolor;
    double bbpinlabelsize; // todo: different default depending on units
    QString sctext;
    bool scpinlabels;
    bool scpinnumbers;
    PropertyMap metadata;
    PropertyMap metaprops;
    QStringList metatags;
    QString filename;
    Part () : units("mm"), width(0), height(0), outline(0.254), color("#116b9e"), corner(0), schematic("edge"),
        mingrid{0,0}, extragrid{0,0}, bbtext("$partnumber"), bbtextcolor("#ffffff"), bbtextsize(5.08),
        bbpinlabels(true), bbpinlabelcolor("#c5e6f9"), bbpinlabelsize(2.54), sctext("$title"), scpinlabels(true),
        scpinnumbers(true), metatags({"fritzpart"}) { }
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
    void on_actLocateMinizip_triggered();
    void on_actNewFile_triggered();
    void updateWindowTitle();
protected:
    void closeEvent(QCloseEvent *event);
private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString curfilename;
    QString basetitle;
    bool promptSaveIfModified ();
    void setCurrentFileName (QString filename) { curfilename = filename; updateWindowTitle(); }
    void saveBasicPart (const Part &part, QString prefix);
    void compile ();
};

#endif // MAINWINDOW_H
