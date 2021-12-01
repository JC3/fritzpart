#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include <QDomDocument>

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

struct Hole { // pcb cutout holes (not pth pin holes)
    double x;
    double y;
    double diameter;
    //double ring; // todo: maybe
    Hole () : x(0), y(0), diameter(0) /*, ring(0)*/ { }
    // temporary parsing context stuff
    bool origleft;
    bool origtop;
};

struct Marking {
    enum Shape { Invalid=0, Circle, Line };
    Shape shape;
    double x1, y1;
    double x2, y2;
    double diam;
    bool capped;
    explicit Marking (Shape shape = Invalid) : shape(shape), x1(0), y1(0), x2(0), y2(0), diam(0),
        capped(true), x1reverse(false), y1reverse(false), x2reverse(false), y2reverse(false),
        xbackoff(false), ybackoff(false) { }
    static Marking makeCircle (double x, double y, double d, bool origleft, bool origtop) {
        Marking m(Circle);
        m.x1 = x;
        m.y1 = y;
        m.diam = d;
        m.origleft = origleft;
        m.origtop = origtop;
        return m;
    }
    static Marking makeLine (double x1, double y1, double x2, double y2, bool origleft, bool origtop) {
        Marking m(Line);
        m.x1 = x1;
        m.y1 = y1;
        m.x2 = x2;
        m.y2 = y2;
        m.origleft = origleft;
        m.origtop = origtop;
        return m;
    }
    // temporary parsing context stuff
    bool origleft;
    bool origtop;
    bool x1reverse, y1reverse;
    bool x2reverse, y2reverse;
    bool xbackoff, ybackoff;
};

// todo: a bunch of things (above) have deferred positions now,
// need to find a cleaner way of handling that.

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
    QList<Hole> pcbholes;
    QList<Marking> pcbmarks;
    double pcbmarkstroke; // todo: different default depending on units
    PropertyMap metadata;
    PropertyMap metaprops;
    QStringList metatags;
    QString filename;
    Part () : units("mm"), width(0), height(0), outline(0.254), color("#116b9e"), corner(0), schematic("edge"),
        mingrid{0,0}, extragrid{0,0}, bbtext("$partnumber"), bbtextcolor("#ffffff"), bbtextsize(5.08),
        bbpinlabels(true), bbpinlabelcolor("#c5e6f9"), bbpinlabelsize(2.54), sctext("$title"), scpinlabels(true),
        scpinnumbers(true), pcbmarkstroke(0.254 * 0.75), metatags({"fritzpart"}) { }
};

struct PartFilenames {
    QString fzpz;        // filename (in cwd) or full path
    QString fzp;         // filename only!!
    QString icon;        // filename only!!
    QString breadboard;  // filename only!!
    QString schematic;   // filename only!!
    QString pcb;         // filename only!!
    explicit PartFilenames (QString prefix = QString(), QString builddir = QString());
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
    void on_actPreview_triggered();
    void on_actCompileTo_triggered();
    void on_actShowOutput_triggered(bool checked);

    void on_actBackup_triggered(bool checked);

protected:
    void closeEvent(QCloseEvent *event);
private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString curfilename;
    QString basetitle;
    bool promptSaveIfModified ();
    void setCurrentFileName (QString filename) { curfilename = filename; updateWindowTitle(); }
    void saveBasicPart (const Part &part, const PartFilenames &names);
    void showPartPreviews (const Part &part);
    void showPartPreviews (const QDomDocument &bb, const QDomDocument &sc, const QDomDocument &pcb);
    Part compile ();
    void clearPartPreviews ();
};

#endif // MAINWINDOW_H
