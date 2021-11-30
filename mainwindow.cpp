#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QDomDocument>
#include <QTemporaryDir>
#include <QProcess>
#include <QDate>
#include <QRegExp>
#include <QCloseEvent>
#include <QSvgRenderer>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    basetitle = windowTitle();
    connect(ui->txtScript->document(), SIGNAL(modificationChanged(bool)), this, SLOT(updateWindowTitle()));
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actOpenFile_triggered()
{
    QString prev = settings.value("scriptpath").toString();
    QString filename = QFileDialog::getOpenFileName(this, "Open File...", prev, "*.txt");
    if (filename != "")
        loadFile(filename);
}

void MainWindow::on_actSaveFile_triggered()
{
    QString prev = settings.value("scriptpath").toString();
    if (curfilename != "")
        prev = curfilename;
    QString filename = QFileDialog::getSaveFileName(this, "Save File...", prev, "*.txt");
    if (filename != "")
        saveFile(filename);
}

void MainWindow::on_actNewFile_triggered()
{
    if (promptSaveIfModified()) {
        //ui->txtScript->setDocument(new QTextDocument(ui->txtScript));
        // whatever:
        ui->txtScript->clear();
        setCurrentFileName("");
        clearPartPreviews();
    }
}

bool MainWindow::promptSaveIfModified() {
    bool confirmed = false;
    if (ui->txtScript->document()->isModified()) {
        int action = QMessageBox::warning(this, "Confirm Action", "there are unsaved changes. do you want to save them?",
                                          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (action == QMessageBox::Save) {
            ui->actSaveFile->trigger();
            confirmed = !ui->txtScript->document()->isModified();
        } else if (action == QMessageBox::Discard) {
            confirmed = true;
        }
    } else {
        confirmed = true;
    }
    return confirmed;
}

void MainWindow::loadFile(QString filename) {
    if (!promptSaveIfModified())
        return;
    try {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly | QFile::Text))
            throw std::runtime_error(file.errorString().toStdString());
        QString script = QString::fromUtf8(file.readAll());
        if (script == "")
            throw std::runtime_error("File contains no text.");
        ui->txtScript->setPlainText(script);
        ui->txtScript->document()->setModified(false);
        setCurrentFileName(QFileInfo(file).absoluteFilePath());
        settings.setValue("scriptpath", QFileInfo(file).absolutePath());
        clearPartPreviews();
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error Loading File", x.what());
    }
}


void MainWindow::saveFile(QString filename) {
    try {
        QFile file(filename);
        if (!file.open(QFile::WriteOnly | QFile::Text))
            throw std::runtime_error(file.errorString().toStdString());
        QByteArray data = ui->txtScript->toPlainText().toUtf8();
        if (file.write(data) != data.length())
            throw std::runtime_error(file.errorString().toStdString());
        setCurrentFileName(QFileInfo(file).absoluteFilePath());
        settings.setValue("scriptpath", QFileInfo(file).absolutePath());
        ui->txtScript->document()->setModified(false);
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error Saving File", x.what());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (promptSaveIfModified())
        event->accept();
    else
        event->ignore();
}

void MainWindow::updateWindowTitle() {
    bool mod = ui->txtScript->document()->isModified();
    QString name = curfilename.isEmpty() ? "untitled" : QFileInfo(curfilename).fileName();
    setWindowTitle(QString("%1%2 - %3").arg(mod ? "*" : "").arg(name, basetitle));
}

void MainWindow::clearPartPreviews () {
    ui->svgBreadboard->load(QByteArray());
    ui->svgPCB->load(QByteArray());
    ui->svgSchematic->load(QByteArray());
}

static bool matches (const QStringList &tokens, QString command, int minparms = -1, int maxparms = -1) {
    if (tokens.empty() || QString::compare(tokens[0], command, Qt::CaseInsensitive))
        return false;
    int parms = tokens.size() - 1;
    if (minparms < 0)
        return true;
    if (parms < minparms)
        return false;
    if (maxparms < 0)
        maxparms = minparms;
    if (parms > maxparms)
        return false;
    return true;
}


static bool matches (const QStringList &tokens, QStringList commands, int minparms = -1, int maxparms = -1) {
    for (const auto &command : commands)
        if (matches(tokens, command, minparms, maxparms))
            return true;
    return false;
}


static double parseCoord (double cur, QString coord) {
    if (coord.startsWith("@"))
        return cur + coord.mid(1).toDouble();
    else
        return coord.toDouble();
}

static bool parseBool (QString str) {
    static QStringList trues = { "true", "yes", "on", "1" };
    static QStringList falses = { "false", "no", "off", "0" };
    if (trues.contains(str, Qt::CaseInsensitive))
        return true;
    else if (falses.contains(str, Qt::CaseInsensitive))
        return false;
    else
        throw std::runtime_error(QString("invalid boolean value: %1").arg(str).toStdString());
}

void MainWindow::on_actCompile_triggered()
{
    try {
        Part part = compile();
        saveBasicPart(part, part.filename);
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error Compiling Part", x.what());
    }
}

void MainWindow::on_actPreview_triggered()
{
    try {
        Part part = compile();
        showPartPreviews(part);
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error Compiling Part", x.what());
    }
}

Part MainWindow::compile() {

    QList<QStringList> scriptlines;
    Part part;

    // ---- tokenize

    QString text = ui->txtScript->toPlainText();
    QTextStream script(&text, QIODevice::ReadOnly);
    QString line;
    bool indesc = false;
    while (script.readLineInto(&line)) {
        // multiline description is special case
        QString tline = line.trimmed();
        if (!indesc && !tline.compare("description:", Qt::CaseInsensitive)) {
            indesc = true;
            continue;
        } else if (indesc && !tline.compare(":description", Qt::CaseInsensitive)) {
            indesc = false;
            continue;
        } else if (indesc) {
            scriptlines.push_back({ "description", tline == "" ? "" : line });
            continue;
        }
        // end description handling
        QStringList tokens;
        std::wistringstream liner(line.toStdWString()); // todo: switch to utf32
        while (!liner.eof()) {
            std::wstring token;
            liner >> std::quoted(token);
            tokens.append(QString::fromStdWString(token));
        }
        while (!tokens.empty() && tokens.back() == "")
            tokens.pop_back();
        if (!tokens.empty() && !tokens[0].startsWith("#"))
            scriptlines.append(tokens);
    }
    if (indesc)
        throw std::runtime_error("end of file in multiline description block");

    // ---- parse

    QStringList metakeys = { "version", "author", "title", "label", "family", "partnumber", "variant", "url", /*"description",*/ "moduleid" };

    double curhole = 0.9, curring = 0.508, curx = 0, cury = 0;
    int curnumber = 1;
    bool origleft = true, origtop = false;
    for (const QStringList &tokens : scriptlines) {
        if (matches(tokens, "units", 1))
            part.units = tokens[1].toLower();
        else if (matches(tokens, "width", 1))
            part.width = tokens[1].toDouble();
        else if (matches(tokens, "height", 1))
            part.height = tokens[1].toDouble();
        else if (matches(tokens, "outline", 1))
            part.outline = tokens[1].toDouble();
        else if (matches(tokens, "hole", 1))
            curhole = tokens[1].toDouble();
        else if (matches(tokens, "ring", 1))
            curring = tokens[1].toDouble();
        else if (matches(tokens, "pin", 2, 4)) {
            Pin pin;
            pin.hole = curhole;
            pin.ring = curring;
            pin.x = parseCoord(curx, tokens[1]);
            pin.y = parseCoord(cury, tokens[2]);
            pin.name = tokens.value(3).trimmed();
            pin.square = !QString::compare(tokens.value(4), "square", Qt::CaseInsensitive);
            pin.number = (curnumber ++);
            pin.origleft = origleft; // have to store and then change origin later since
            pin.origtop = origtop;   // width / height may not have been defined yet.
            part.pins.append(pin);
            curx = pin.x;
            cury = pin.y;
        } else if (matches(tokens, "color", 1))
            part.color = tokens[1];
        else if (matches(tokens, "corner", 1))
            part.corner = tokens[1].toDouble();
        else if (matches(tokens, "schematic", 1, 2)) {
            part.schematic = tokens[1].toLower();
            part.schematicmod = (tokens.size() > 2 ? tokens[2].toLower() : "");
        } else if (matches(tokens, "scminsize", 2)) {
            part.mingrid[0] = tokens[1].toInt() - 1;
            part.mingrid[1] = tokens[2].toInt() - 1;
        } else if (matches(tokens, "scgrow", 2)) {
            part.extragrid[0] = abs(tokens[1].toInt());
            part.extragrid[1] = abs(tokens[2].toInt());
        } else if (matches(tokens, "sctext", 1)) {
            part.sctext = tokens[1];
        } else if (matches(tokens, "sclabels", 1)) {
            part.scpinlabels = parseBool(tokens[1]);
        } else if (matches(tokens, "scnumbers", 1)) {
            part.scpinlabels = parseBool(tokens[1]);
        } else if (matches(tokens, "bbtext", 1, 3)) {
            part.bbtext = tokens[1];
            if (tokens.size() > 2) part.bbtextcolor = tokens[2];
            if (tokens.size() > 3) part.bbtextsize = tokens[3].toDouble();
        } else if (matches(tokens, "bblabels", 1, 3)) {
            part.bbpinlabels = parseBool(tokens[1]);
            if (tokens.size() > 2) part.bbpinlabelcolor = tokens[2];
            if (tokens.size() > 3) part.bbpinlabelsize = tokens[3].toDouble();
        } else if (matches(tokens, "origin", 1, INT_MAX)) {
            for (int n = 1; n < tokens.size(); ++ n) {
                if (tokens[n].startsWith("l", Qt::CaseInsensitive))
                    origleft = true;
                else if (tokens[n].startsWith("r", Qt::CaseInsensitive))
                    origleft = false;
                else if (tokens[n].startsWith("t", Qt::CaseInsensitive))
                    origtop = true;
                else if (tokens[n].startsWith("b", Qt::CaseInsensitive))
                    origtop = false;
            }
        } else if (matches(tokens, metakeys, 1)) {
            part.metadata[tokens[0].toLower()] = tokens[1].trimmed();
        } else if (matches(tokens, "description", 0, 1)) {
            part.metadata["description"] = part.metadata["description"] + tokens.value(1) + "\n";
        } else if (matches(tokens, "filename", 1))
            part.filename = tokens[1];
        else if (matches(tokens, "property", 1, 2))
            part.metaprops[tokens[1]] = tokens.value(2);
        else if (matches(tokens, "tag", 1, INT_MAX) || matches(tokens, "tags", 1, INT_MAX)) {
            for (int n = 1; n < tokens.size(); ++ n)
                part.metatags.append(tokens[n]);
        } else
            throw std::runtime_error(QString("unknown directive: %1").arg(tokens.join(",")).toStdString());
    }

    for (Pin &pin : part.pins) { // now that we probably have width/height, apply origin settings
        if (!pin.origleft) {
            pin.origleft = true;
            pin.x = part.width - pin.x;
        }
        if (!pin.origtop) {
            pin.origtop = true;
            pin.y = part.height - pin.y;
        }
    }

    // ---- fill in some defaults

    auto getany = [&part](QStringList sourcekeys, QString planb) {
        for (const QString &s : sourcekeys)
            if (part.metadata.value(s) != "")
                return part.metadata[s];
        return planb;
    };

    auto setdef = [&](QString key, QStringList sourcekeys, QString planb = QString()) {
        if (part.metadata[key] == "")
            part.metadata[key] = getany(sourcekeys, planb);
    };

    auto metaval = [&part](QString value) {
        if (value.startsWith("$"))
            return part.metadata.value(value.mid(1).trimmed().toLower(), "???");
        else
            return value;
    };

    part.metadata["description"] = part.metadata["description"].trimmed();

    if (part.filename == "")
        part.filename = getany({ "moduleid", "title", "partnumber", "family" }, "compiled");
    setdef("partnumber", { "title", "family" }, part.filename);
    setdef("family", { "partnumber" });
    setdef("title", { "partnumber", "family" });
    setdef("variant", { "partnumber" }, "variant 1");
    setdef("description", { "title" });
    setdef("version", { }, "1");
    setdef("label", { }, "U");
    setdef("moduleid", { }, part.filename);
    // url can be left blank

    part.sctext = metaval(part.sctext);
    part.bbtext = metaval(part.bbtext);

    // ----

    qDebug() << "size" << part.width << part.height << part.units;
    qDebug() << "outline" << part.outline;
    for (const Pin &pin : part.pins)
        qDebug() << "  pin" << pin.number << pin.name << "@" << pin.x << pin.y << "d=" << pin.hole << "r=" << pin.ring << (pin.square ? "square" : "round");

    return part;

}


static QDomElement initDocument (QDomDocument &doc, QString root) {
    QDomProcessingInstruction dec = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"");
    QDomComment info = doc.createComment("Generated by fritzpart.");
    QDomElement node = doc.createElement(root);
    // hack alert
    if (root == "svg")
        node.setAttribute("xmlns", "http://www.w3.org/2000/svg");
    // moving on...
    doc.appendChild(dec);
    doc.appendChild(info);
    doc.appendChild(node);
    return node;
}


static QDomElement appendElement (QDomNode node, QString tag, QString id = QString()) {
    QDomElement el = node.ownerDocument().createElement(tag);
    if (id != "")
        el.setAttribute("id", id);
    node.appendChild(el);
    return el;
}


static QDomElement appendElement (QDomNode node, QDomElement el) {
    node.appendChild(el);
    return el;
}


template <typename T>
static QString pretty (T p) {
    return QString("%1").arg(p);
}

struct SVGStyle {
    QString fill;
    QString stroke;
    double strokeWidth;
};

static QDomElement svgLine (QDomDocument doc, QString id, double x1, double y1, double x2, double y2, const SVGStyle &style, bool roundCaps = false) {
    QDomElement line = doc.createElement("line");
    if (id != "")
        line.setAttribute("id", id);
    line.setAttribute("x1", x1);
    line.setAttribute("y1", y1);
    line.setAttribute("x2", x2);
    line.setAttribute("y2", y2);
    line.setAttribute("fill", style.fill);
    line.setAttribute("stroke", style.stroke);
    line.setAttribute("stroke-width", pretty(style.strokeWidth));
    if (roundCaps)
        line.setAttribute("stroke-linecap", "round");
    return line;
}

static QDomElement svgRect (QDomDocument doc, QString id, double x, double y, double w, double h, const SVGStyle &style, bool borderInside = false) {
    QDomElement rect = doc.createElement("rect");
    if (id != "")
        rect.setAttribute("id", id);
    rect.setAttribute("fill", style.fill);
    rect.setAttribute("stroke", style.stroke);
    rect.setAttribute("stroke-width", pretty(style.strokeWidth));
    if (borderInside) {
        rect.setAttribute("x", pretty(x + style.strokeWidth / 2.0));
        rect.setAttribute("y", pretty(y + style.strokeWidth / 2.0));
        rect.setAttribute("width", pretty(w - style.strokeWidth));
        rect.setAttribute("height", pretty(h - style.strokeWidth));
    } else {
        rect.setAttribute("x", pretty(x - style.strokeWidth / 2.0));
        rect.setAttribute("y", pretty(y - style.strokeWidth / 2.0));
        rect.setAttribute("width", pretty(w + style.strokeWidth));
        rect.setAttribute("height", pretty(h + style.strokeWidth));
    }
    return rect;
}

static QDomElement svgCircle (QDomDocument doc, QString id, double cx, double cy, double r, const SVGStyle &style, bool borderInside = false) {
    QDomElement circle = doc.createElement("circle");
    if (id != "")
        circle.setAttribute("id", id);
    circle.setAttribute("fill", style.fill);
    circle.setAttribute("stroke", style.stroke);
    circle.setAttribute("stroke-width", pretty(style.strokeWidth));
    circle.setAttribute("cx", pretty(cx));
    circle.setAttribute("cy", pretty(cy));
    if (borderInside)
        circle.setAttribute("r", pretty(r - style.strokeWidth / 2.0));
    else
        circle.setAttribute("r", pretty(r + style.strokeWidth / 2.0));
    return circle;
}

enum SVGTextAlign { LeftAlign, CenterAlign, RightAlign, BottomCenterAlign, TopCenterAlign };

struct SVGTextStyle {
    QString color;
    double size;
};

static const char * svgTextAnchor (SVGTextAlign align) {
    switch (align) {
    case LeftAlign: return "start";
    case TopCenterAlign: return "middle";
    case BottomCenterAlign: return "middle";
    case CenterAlign: return "middle";
    case RightAlign: return "end";
    default: return "";
    }
}


static QDomElement svgText (QDomDocument doc, QString content, double x, double y, const SVGTextStyle &style, SVGTextAlign align, double rotate = 0) {
    QDomElement text = doc.createElement("text");
    text.setAttribute("font-family", "'Droid Sans'");
    text.setAttribute("stroke", "none");
    text.setAttribute("stroke-width", 0);
    text.setAttribute("fill", style.color);
    text.setAttribute("font-size", pretty(style.size));
    // Droid Sans cap-height / 2 = 0.357  (also x-height / 2 = 0.268)
    double voffset = style.size * 0.357;
    if (align == BottomCenterAlign)
        voffset = 0;
    else if (align == TopCenterAlign)
        voffset = style.size;
    if (fabs(rotate) > 1e-5) {
        QString voffsettr = (fabs(voffset > 1e-5) ? QString(" translate(0,%1)").arg(voffset) : "");
        text.setAttribute("transform", QString("translate(%1,%2) rotate(%3)%4")
                          .arg(x).arg(y).arg(rotate).arg(voffsettr));
    } else {
        text.setAttribute("x", pretty(x));
        text.setAttribute("y", pretty(y + voffset));
    }
    text.setAttribute("text-anchor", svgTextAnchor(align));
    //text.setAttribute("dominant-baseline", "middle"); // fritzing ignores this :(
    //text.setAttribute("dy", "0.5ex"); // it ignores dy too
    // ^ see https://github.com/fritzing/fritzing-app/issues/3909
    text.appendChild(doc.createTextNode(content));
    return text;
}


static QDomDocument generatePCB (const Part &part) {

    QDomDocument svg;
    QDomElement root = initDocument(svg, "svg");
    QDomElement silkscreen = appendElement(root, "g", "silkscreen");
    QDomElement copper = appendElement(appendElement(root, "g", "copper0"), "g", "copper1");

    root.setAttribute("version", "1.1");
    root.setAttribute("x", 0);
    root.setAttribute("y", 0);
    root.setAttribute("width", QString("%1%2").arg(part.width).arg(part.units));
    root.setAttribute("height", QString("%1%2").arg(part.height).arg(part.units));
    root.setAttribute("viewBox", QString("0 0 %1 %2").arg(part.width).arg(part.height));
    root.setAttribute("id", "svg");

    if (part.outline > 0) {
        SVGStyle stsilk = { "none", "#000000", part.outline };
        silkscreen.appendChild(svgRect(svg, "outline", 0, 0, part.width, part.height, stsilk, true));
    }

    for (const Pin &pin : part.pins) {
        double r = pin.hole / 2.0;
        QString id = QString("connector%1pin").arg(pin.number - 1);
        SVGStyle stpad = { "none", "#f7bd13", pin.ring };
        QDomElement circle = svgCircle(svg, "", pin.x, pin.y, r, stpad);
        QDomElement pad;
        if (pin.square) {
            circle.setAttribute("id", id + "_circle");
            QDomElement square = svgRect(svg, id + "_square", pin.x - r, pin.y - r, pin.hole, pin.hole, stpad);
            QDomElement group = svg.createElement("g");
            group.appendChild(square);
            group.appendChild(circle);
            pad = group;
        } else {
            pad = circle;
        }
        pad.setAttribute("id", id);
        copper.appendChild(pad);
    }

    return svg;

}


static QDomDocument generateBreadboard (const Part &part, QString layername = "breadboard" /* for now, while we're using it for icon too */) {

    QDomDocument svg;
    QDomElement root = initDocument(svg, "svg");
    QDomElement bboard = appendElement(root, "g", layername);

    root.setAttribute("version", "1.1");
    root.setAttribute("x", 0);
    root.setAttribute("y", 0);
    root.setAttribute("width", QString("%1%2").arg(part.width).arg(part.units));
    root.setAttribute("height", QString("%1%2").arg(part.height).arg(part.units));
    root.setAttribute("viewBox", QString("0 0 %1 %2").arg(part.width).arg(part.height));
    root.setAttribute("id", "svg");

    if (part.outline > 0) {
        SVGStyle st = { part.color, "#000000", part.outline };
        QDomElement rect = appendElement(bboard, svgRect(svg, "part", 0, 0, part.width, part.height, st, true));
        if (part.corner > 0) {
            rect.setAttribute("rx", part.corner);
            rect.setAttribute("ry", part.corner);
        }
    }

    for (const Pin &pin : part.pins) {
        QDomElement conn = appendElement(bboard, "g");
        conn.setAttribute("transform", QString("translate(%1,%2)").arg(pin.x).arg(pin.y));
        double r = pin.hole / 2.0;
        QString id = QString("connector%1pin").arg(pin.number - 1);
        SVGStyle stpad = { "#8c8c8c", "none", 0 };
        QDomElement pad;
        if (pin.square)
            pad = svgRect(svg, id, -r, -r, pin.hole, pin.hole, stpad);
        else
            pad = svgCircle(svg, id, 0, 0, r, stpad);
        conn.appendChild(pad);
        if (part.bbpinlabels && pin.name != "") {
            SVGTextStyle tstlabel = { part.bbpinlabelcolor, part.bbpinlabelsize };
            const double inset = r + 0.35 * part.bbpinlabelsize; // i guess.
            // sloppily find closest edge
            struct EdgeMetrics { double e, dx, dy, rot; SVGTextAlign align; } metrics[] = {
              { fabs(pin.x), 1, 0, 0, LeftAlign },
              { fabs(part.width - pin.x), -1, 0, 0, RightAlign },
              { fabs(pin.y), 0, 1, -90, RightAlign },
              { fabs(part.height - pin.y), 0, -1, -90, LeftAlign }
            };
            EdgeMetrics *m = std::min_element(metrics, metrics + 4, [](auto &a, auto &b){return a.e<b.e;});
            // well that was the weirdest code i've written in a while.
            // todo: need a better way to control where these end up
            conn.appendChild(svgText(svg, pin.name, m->dx*inset, m->dy*inset, tstlabel, m->align, m->rot));
        }
    }

    if (part.bbtext != "") {
        SVGTextStyle tstpart = { part.bbtextcolor, part.bbtextsize };
        bboard.appendChild(svgText(svg, part.bbtext, part.width / 2.0, part.height / 2.0, tstpart, CenterAlign));
    }

    return svg;

}


static QDomDocument generateIcon (const Part &part) {

    // just use breadboard image for now
    return generateBreadboard(part, "icon");

}

enum ScEdge { NoEdge = 0, Top, Bottom, Left, Right };

struct ScPin {
    int number;
    int gridpos;
    ScEdge edge;
    QString name;
    double pinpos;
    ScPin () : number(-1), gridpos(-1), edge(NoEdge), pinpos(0) { }
    explicit ScPin (const Pin &pin) : number(pin.number), gridpos(-1), edge(NoEdge), name(pin.name), pinpos(0) { }
};

enum ScStyle { Box, Header };
enum ScHeaderStyle { Terminal, Male, Female };
enum ScEdgeMode { HEdge, VEdge, HVEdge };

struct ScPart {
    int gridw;
    int gridh;
    QList<ScPin> pins;
    bool haslpins;
    bool hasrpins;
    bool hastpins;
    bool hasbpins;
    ScPart () : gridw(0), gridh(0), haslpins(false), hasrpins(false), hastpins(false), hasbpins(false) { }
};

static ScPart scPlaceEdge (const Part &part, ScEdgeMode mode) {

    ScPart sc;

    // ---- figure out quadrant and edge of pins

    QList<ScPin> lpins[2], rpins[2], tpins[2], bpins[2];
    for (const Pin &pin : part.pins) {
        ScPin scpin(pin);
        double ldist = fabs(pin.x);
        double rdist = fabs(part.width - pin.x);
        double tdist = fabs(pin.y);
        double bdist = fabs(part.height - pin.y);
        bool h;
        // should the pin be horizontal or vertical?
        if (mode == HVEdge)
            h = qMin(ldist, rdist) < qMin(tdist, bdist);
        else
            h = (mode == HEdge);
        // [0] is top or left half of edge, [1] is bottom or right half
        if (h) {
            scpin.pinpos = pin.y;
            (ldist < rdist ? lpins : rpins)[tdist < bdist ? 0 : 1].append(scpin);
        } else {
            scpin.pinpos = pin.x;
            (tdist < bdist ? tpins : bpins)[ldist < rdist ? 0 : 1].append(scpin);
        }
    }

    // ---- now pack all the pins into the grid

    auto addpins = [](QList<ScPin> &scpins, QList<ScPin> pins[2], int nslots, ScEdge edge) {
        assert(nslots >= pins[0].size() + pins[1].size());
        // stable sort so pins at same location stay sorted by number.
        std::stable_sort(pins[0].begin(), pins[0].end(), [](auto &a,auto &b){return a.pinpos<b.pinpos;}); // ascending!
        std::stable_sort(pins[1].begin(), pins[1].end(), [](auto &a,auto &b){return b.pinpos<a.pinpos;}); // descending!
        int pos = 0;
        for (ScPin pin : pins[0]) {
            pin.edge = edge;
            pin.gridpos = (pos ++);
            scpins.append(pin);
        }
        pos = nslots;
        for (ScPin pin : pins[1]) {
            pin.edge = edge;
            pin.gridpos = (-- pos);
            scpins.append(pin);
        }
        return (pins[0].size() + pins[1].size()) > 0;
    };

    sc.gridw = qMax(tpins[0].size() + tpins[1].size(), bpins[0].size() + bpins[1].size()) + part.extragrid[0];
    sc.gridh = qMax(lpins[0].size() + lpins[1].size(), rpins[0].size() + rpins[1].size()) + part.extragrid[1];
    sc.gridw = qMax(sc.gridw, part.mingrid[0]);
    sc.gridh = qMax(sc.gridh, part.mingrid[1]);
    sc.hastpins = addpins(sc.pins, tpins, sc.gridw, Top);
    sc.hasbpins = addpins(sc.pins, bpins, sc.gridw, Bottom);
    sc.haslpins = addpins(sc.pins, lpins, sc.gridh, Left);
    sc.hasrpins = addpins(sc.pins, rpins, sc.gridh, Right);
    // this sort isnt necessary, it's just to keep the svg a little more readable
    std::sort(sc.pins.begin(), sc.pins.end(), [](auto &a,auto &b){return a.number<b.number;});

    return sc;

}


static ScPart scPlaceLinear (const Part &part) {

    ScPart sc;

    int curpos = 0;
    for (const Pin &pin : part.pins) {
        ScPin scpin(pin);
        scpin.gridpos = (curpos ++);
        scpin.edge = Left;
        sc.pins.append(scpin);
    }

    sc.gridw = 0;
    sc.gridh = curpos;
    sc.haslpins = true;

    return sc;

}


static QDomDocument generateSchematic (const Part &part) {

    // ---- generate schematic

    ScPart sc;
    ScStyle style = Box;
    ScHeaderStyle hdrstyle = Terminal;
    if (part.schematic == "hedge")
        sc = scPlaceEdge(part, HEdge);
    else if (part.schematic == "vedge")
        sc = scPlaceEdge(part, VEdge);
    else if (part.schematic == "edge")
        sc = scPlaceEdge(part, HVEdge);
    else if (part.schematic == "header") {
        sc = scPlaceLinear(part);
        style = Header;
        if (part.schematicmod == "male")
            hdrstyle = Male;
        else if (part.schematicmod == "female")
            hdrstyle = Female;
        else if (part.schematicmod == "terminal" || part.schematicmod == "")
            hdrstyle = Terminal;
        else
            throw std::runtime_error(QString("unknown schematic header type: %1").arg(part.schematicmod).toStdString());
    } else if (part.schematic == "block")
        sc = scPlaceLinear(part);
    else
        throw std::runtime_error(QString("unknown schematic type: %1").arg(part.schematic).toStdString());

    qDebug() << "schematic:" << sc.gridw << "x" << sc.gridh;
    for (const ScPin &pin : sc.pins)
        qDebug() << pin.number << pin.edge << pin.name << pin.pinpos << pin.gridpos;

    // ---- generate svg from schematic
#define PIN_CAPS 1

    QDomDocument svg;
    QDomElement root = initDocument(svg, "svg");
    root.setAttribute("version", "1.1");
    root.setAttribute("id", "svg");

    const SVGStyle stline = { "none", "#000000", 0.7 / 7.2 };
    const SVGStyle stpin = { "none", "#555555", 0.7 / 7.2 };
    const SVGStyle stterm = { "none", "none", 0 };
    const SVGTextStyle tstpart = { "#000000", 10.0 * 4.25 / 72.0 };
    const SVGTextStyle tstpin = { "#555555", 10.0 * 3.5 / 72.0 };
    const SVGTextStyle tstnum = { "#555555", 10.0 * 2.5 / 72.0 };
    constexpr double PinLabelInset = 0.15; // not in graphics standard
    constexpr double PinNumberOffset = 0.1; // not in graphics standard

    if (style == Header) {

        // ==== header style

        // build viewbox as we go; todo: also do this for Box schematics below. i wrote this
        // Header bit after the Box bit so this is a litte cleaner.
        QRectF rcbox;
        const double halfstr = stline.strokeWidth / 2.0;

        QDomElement schem = appendElement(root, "g", "schematic");
        QDomElement bg = appendElement(schem, "g", "background");
        QDomElement pins = appendElement(schem, "g", "pins");

        // male/female pin metrics from fritzing's generic_[fe]male_pin_headers.
        // terminal metrics from fritzing's camdenboss connectors (roughly).
        constexpr double PHSize = (2.0 - 9.928 / 7.2), PVSize = (3.6 - 1.643) / 7.2;
        constexpr double TRadius = 2.9 / 7.2;
        // the 2.9's on the next line don't need to match the one above
        constexpr double TBoxHDist = (0.252 + 12.2 - 7.072 - 2.9) / 7.2, TBoxVDist = (4.5 - 2.9) / 7.2; // to outer edge

        for (const ScPin &pin : sc.pins) {
            QString idpref = QString("connector%1").arg(pin.number - 1);
            QDomElement conn = appendElement(pins, "g", idpref);
            // - - generate pins and decorations in the box (0,-.5) - (2,.5)
            conn.appendChild(svgRect(svg, idpref + "terminal", 0, 0, 1e-5, 1e-5, stterm));
            conn.appendChild(svgLine(svg, idpref + "pin", 0, 0, 1, 0, stpin, PIN_CAPS ? true : false));
            if (hdrstyle == Male) {
                conn.appendChild(svgLine(svg, "", 1.0, 0, 2.0, 0, stline));
                conn.appendChild(svgLine(svg, "", 2.0, 0, 2.0 - PHSize, PVSize, stline, true));
                conn.appendChild(svgLine(svg, "", 2.0, 0, 2.0 - PHSize, -PVSize, stline, true));
            } else if (hdrstyle == Female) {
                conn.appendChild(svgLine(svg, "", 1.0, 0, 2.0 - PHSize, 0, stline));
                conn.appendChild(svgLine(svg, "", 2.0 - PHSize, 0, 2.0, PVSize, stline, true));
                conn.appendChild(svgLine(svg, "", 2.0 - PHSize, 0, 2.0, -PVSize, stline, true));
            } else if (hdrstyle == Terminal) {
                conn.appendChild(svgLine(svg, "", 1.0, 0, 2.0 - 2.0 * TRadius, 0, stline));
                conn.appendChild(svgCircle(svg, "", 2.0 - TRadius, 0, TRadius + 0.5*stline.strokeWidth /* bah */, stline, true));
            }
            if (part.scpinnumbers)
                conn.appendChild(svgText(svg, QString("%1").arg(pin.number), 0.5, -0.5*stpin.strokeWidth - PinNumberOffset, tstnum, BottomCenterAlign));
            // - - set position
            conn.setAttribute("transform", QString("translate(0,%1)").arg(pin.gridpos));
            rcbox = QRectF(0, -0.5, 2, 1)
                    .adjusted(-halfstr, -halfstr, halfstr, halfstr)
                    .translated(0, pin.gridpos)
                    .united(rcbox);
        }

        if (hdrstyle == Terminal) {
            QRectF rcblock = QRectF(QPointF(-TRadius, -TRadius), QPointF(TRadius, sc.gridh+TRadius-1))
                    .adjusted(-TBoxHDist, -TBoxVDist, TBoxHDist, TBoxVDist)
                    .translated(2.0 - TRadius, 0.0);
            bg.appendChild(svgRect(svg, "block", rcblock.x(), rcblock.y(), rcblock.width(), rcblock.height(), stline, true));
            rcbox = rcbox.united(rcblock);
        }

        root.setAttribute("x", 0);
        root.setAttribute("y", 0);
        root.setAttribute("width", QString("%1in").arg(rcbox.width() * 0.1));
        root.setAttribute("height", QString("%1in").arg(rcbox.height() * 0.1));
        root.setAttribute("viewBox", QString("%1 %2 %3 %4")
                          .arg(rcbox.x()).arg(rcbox.y()).arg(rcbox.width()).arg(rcbox.height()));

        if (!bg.hasChildNodes()) // drop the background group if we didn't use it for anything
            bg.parentNode().removeChild(bg);

        // ==== end header style

    } else {

        // ==== box style

        QRect rcbox(-1, -1, qMax(1, sc.gridw) + 1, qMax(1, sc.gridh) + 1);
        QRectF rcpart = rcbox.adjusted(sc.haslpins ? -1 : 0,
                                       sc.hastpins ? -1 : 0,
                                       sc.hasrpins ? 1 : 0,
                                       sc.hasbpins ? 1 : 0);

#if PIN_CAPS
        rcpart.adjust(-stpin.strokeWidth / 2.0, -stpin.strokeWidth / 2.0,
                      stpin.strokeWidth / 2.0, stpin.strokeWidth / 2.0);
#endif

        root.setAttribute("x", 0);
        root.setAttribute("y", 0);
        root.setAttribute("width", QString("%1in").arg(rcpart.width() * 0.1));
        root.setAttribute("height", QString("%1in").arg(rcpart.height() * 0.1));
        root.setAttribute("viewBox", QString("%1 %2 %3 %4")
                          .arg(rcpart.x()).arg(rcpart.y())
                          .arg(rcpart.width()).arg(rcpart.height()));

        QDomElement schem = appendElement(root, "g", "schematic");
        QDomElement pins = appendElement(schem, "g", "pins");
        QDomElement labels = appendElement(schem, "g", "labels");

        for (const ScPin &scpin : sc.pins) {
            QPoint p1, p2, pt;
            QPointF pl, pn;
            SVGTextAlign la = CenterAlign;
            double lr = 0;
            if (scpin.edge == Top) {
                pt = p1 = QPoint(scpin.gridpos, rcbox.top() - 1);
                pl = p2 = QPoint(scpin.gridpos, rcbox.top());
                la = RightAlign;
                lr = -90;
                pl += QPointF(0, stline.strokeWidth + PinLabelInset);
                pn = QPointF(p1 + p2) / 2.0 - QPointF(stpin.strokeWidth / 2.0 + PinNumberOffset, 0);
            } else if (scpin.edge == Bottom) {
                pl = p1 = QPoint(scpin.gridpos, rcbox.bottom() + 1);
                pt = p2 = QPoint(scpin.gridpos, rcbox.bottom() + 2);
                la = LeftAlign;
                lr = -90;
                pl -= QPointF(0, stline.strokeWidth + PinLabelInset);
                pn = QPointF(p1 + p2) / 2.0 - QPointF(stpin.strokeWidth / 2.0 + PinNumberOffset, 0);
            } else if (scpin.edge == Left) {
                pt = p1 = QPoint(rcbox.left() - 1, scpin.gridpos);
                pl = p2 = QPoint(rcbox.left(), scpin.gridpos);
                la = LeftAlign;
                pl += QPointF(stline.strokeWidth + PinLabelInset, 0);
                pn = QPointF(p1 + p2) / 2.0 - QPointF(0, stpin.strokeWidth / 2.0 + PinNumberOffset);
            } else if (scpin.edge == Right) {
                pl = p1 = QPoint(rcbox.right() + 1, scpin.gridpos);
                pt = p2 = QPoint(rcbox.right() + 2, scpin.gridpos);
                la = RightAlign;
                pl -= QPointF(stline.strokeWidth + PinLabelInset, 0);
                pn = QPointF(p1 + p2) / 2.0 - QPointF(0, stpin.strokeWidth / 2.0 + PinNumberOffset);
            }
            QDomElement term = appendElement(pins, svgRect(svg, QString("connector%1terminal").arg(scpin.number - 1), pt.x(), pt.y(), 1e-5, 1e-5, stterm));
            //term.setAttribute("class", "terminal");
            QDomElement conn = appendElement(pins, "line", QString("connector%1pin").arg(scpin.number - 1));
            // todo: you can use svgLine now
            conn.setAttribute("x1", p1.x());
            conn.setAttribute("y1", p1.y());
            conn.setAttribute("x2", p2.x());
            conn.setAttribute("y2", p2.y());
            conn.setAttribute("fill", stpin.fill);
            conn.setAttribute("stroke", stpin.stroke);
            conn.setAttribute("stroke-width", pretty(stpin.strokeWidth));
#if PIN_CAPS
            conn.setAttribute("stroke-linecap", "round");
#endif
            //conn.setAttribute("class", "pin");
            if (part.scpinlabels && scpin.name != "")
                labels.appendChild(svgText(svg, scpin.name, pl.x(), pl.y(), tstpin, la, lr));
            if (part.scpinnumbers)
                labels.appendChild(svgText(svg, QString("%1").arg(scpin.number), pn.x(), pn.y(), tstnum, BottomCenterAlign, lr));
            // todo: utility function to generate a pin; origin at part-side point, then use
            // transform(rotate) for vertical ones.
        }

        QDomElement outline = appendElement(schem, svgRect(svg, "outline", rcbox.x(), rcbox.y(),
                                                           rcbox.width(), rcbox.height(), stline, true));
        //outline.setAttribute("rx", stline.strokeWidth / 2.0);
        //outline.setAttribute("ry", stline.strokeWidth / 2.0);

        if (part.sctext != "") {
            QPointF center = QRectF(rcbox).center();
            if (part.schematic == "block") // todo: really need to change those QRects to QRectFs.
                labels.appendChild(svgText(svg, part.sctext, 1 + rcbox.right() - PinLabelInset, center.y(), tstpart, TopCenterAlign, 90));
            else
                labels.appendChild(svgText(svg, part.sctext, center.x(), center.y(), tstpart, CenterAlign));
        }

        // === end box style

    }

    return svg;

}


static QDomElement appendSimple (QDomNode parent, QString tag, QString text) {
    QDomElement el = parent.ownerDocument().createElement(tag);
    el.appendChild(parent.ownerDocument().createTextNode(text));
    parent.appendChild(el);
    return el;
}


static QDomDocument generateFZP (const Part &part, QString prefix) {

    QDomDocument fzp;
    QDomElement module = initDocument(fzp, "module");
    module.setAttribute("referenceFile", QString("%1.fzp").arg(prefix));
    module.setAttribute("fritzingVersion", "0.9.9");
    module.setAttribute("moduleId", prefix);

    appendSimple(module, "version", part.metadata["version"]);
    appendSimple(module, "author", part.metadata["author"]);
    appendSimple(module, "title", part.metadata.getValue("title", prefix));
    appendSimple(module, "label", part.metadata["label"]);
    appendSimple(module, "date", QDate::currentDate().toString());
    //appendSimple(module, "taxonomy", QString("part.dip.%1.pins").arg(part.pins.size())); // todo: ???
    appendSimple(module, "description", part.metadata["description"]);
    appendSimple(module, "url", part.metadata["url"]);

    QDomElement tags = appendElement(module, "tags");
    for (const QString &tag : part.metatags)
        appendSimple(tags, "tag", tag);

    // todo: fix the case-sensitive weirdness lurking in here
    PropertyMap outprops = part.metaprops;
    outprops["family"] = part.metadata.getValue("family", outprops["family"]);
    outprops["variant"] = part.metadata.getValue("variant", outprops["variant"]);
    outprops["part number"] = part.metadata.getValue("partnumber", outprops["part number"]);

    QDomElement props = appendElement(module, "properties");
    /*
    appendSimple(props, "property", part.metadata.getValue("family", prefix)).setAttribute("name", "family");
    appendSimple(props, "property", part.metadata["variant"]).setAttribute("name", "variant");
    appendSimple(props, "property", part.metadata.getValue("partnumber", prefix)).setAttribute("name", "part number");
    */
    for (auto pv = outprops.cbegin(); pv != outprops.cend(); ++ pv)
        appendSimple(props, "property", pv.value()).setAttribute("name", pv.key());

    {
        QDomElement views = appendElement(module, "views"), view, layers;
        view = appendElement(views, "iconView");
        layers = appendElement(view, "layers");
        layers.setAttribute("image", QString("icon/%1_icon.svg").arg(prefix));
        appendElement(layers, "layer").setAttribute("layerId", "icon");
        view = appendElement(views, "breadboardView");
        layers = appendElement(view, "layers");
        layers.setAttribute("image", QString("breadboard/%1_breadboard.svg").arg(prefix));
        appendElement(layers, "layer").setAttribute("layerId", "breadboard");
        view = appendElement(views, "schematicView");
        layers = appendElement(view, "layers");
        layers.setAttribute("image", QString("schematic/%1_schematic.svg").arg(prefix));
        appendElement(layers, "layer").setAttribute("layerId", "schematic");
        view = appendElement(views, "pcbView");
        layers = appendElement(view, "layers");
        layers.setAttribute("image", QString("pcb/%1_pcb.svg").arg(prefix));
        appendElement(layers, "layer").setAttribute("layerId", "silkscreen");
        appendElement(layers, "layer").setAttribute("layerId", "copper0");
        appendElement(layers, "layer").setAttribute("layerId", "copper1");
    }

    auto addp = [](QDomNode view, QString layer, int number, bool terminal) {
        QDomElement p = appendElement(view, "p");
        p.setAttribute("layer", layer);
        p.setAttribute("svgId", QString("connector%1pin").arg(number - 1));
        if (terminal)
            p.setAttribute("terminalId", QString("connector%1terminal").arg(number - 1));
    };

    QDomElement conns = appendElement(module, "connectors");
    for (const Pin &pin : part.pins) {
        QString name = (pin.name == "" ? QString("pin %1").arg(pin.number) : pin.name);
        QDomElement conn = appendElement(conns, "connector");
        conn.setAttribute("name", name);
        conn.setAttribute("id", QString("connector%1").arg(pin.number - 1));
        conn.setAttribute("type", "male");
        appendSimple(conn, "description", name);
        QDomElement views = appendElement(conn, "views"), view;
        view = appendElement(views, "breadboardView");
        addp(view, "breadboard", pin.number, false);
        view = appendElement(views, "schematicView");
        addp(view, "schematic", pin.number, true);
        view = appendElement(views, "pcbView");
        addp(view, "copper0", pin.number, false);
        addp(view, "copper1", pin.number, false);
    }

    qDebug().noquote() << fzp.toString();
    return fzp;

}


static void writeXML (QDomDocument doc, QString filename) {
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        throw std::runtime_error(file.errorString().toStdString());
    QTextStream text(&file);
    text.setCodec("utf-8");
    doc.save(text, 2, QDomNode::EncodingFromTextStream);
    qDebug() << "saved" << filename;
}


static QString sanitize (QString filename) {
    // super picky, and latin chars only
    filename = filename
            .trimmed()
            .replace(QRegExp("[ ]+"), " ")
            .replace(QRegExp("[^a-zA-Z0-9_-]"), "_");
    return (filename == "") ? "compiled" : filename;
}


void MainWindow::saveBasicPart(const Part &part, QString prefix) {

    prefix = sanitize(prefix);

    QDomDocument pcb = generatePCB(part);
    QDomDocument breadboard = generateBreadboard(part);
    QDomDocument schematic = generateSchematic(part);
    QDomDocument icon = generateIcon(part);
    QDomDocument fzp = generateFZP(part, prefix);

#if 1 // debugging
    writeXML(pcb, QString("%1-pcb.svg").arg(prefix));
    writeXML(breadboard, QString("%1-breadboard.svg").arg(prefix));
    writeXML(schematic, QString("%1-schematic.svg").arg(prefix));
    writeXML(icon, QString("%1-icon.svg").arg(prefix));
    writeXML(fzp, QString("%1.fzp").arg(prefix));
#endif

    QList<QPair<QDomDocument,QString> > files = {
        { pcb, QString("svg.pcb.%1_pcb.svg").arg(prefix) },
        { breadboard, QString("svg.breadboard.%1_breadboard.svg").arg(prefix) },
        { schematic, QString("svg.schematic.%1_schematic.svg").arg(prefix) },
        { icon, QString("svg.icon.%1_icon.svg").arg(prefix) },
        { fzp, QString("part.%1.fzp").arg(prefix) }
    };

    QTemporaryDir workdir;
    if (!workdir.isValid())
        throw std::runtime_error("failed to create temporary work directory");

    QStringList filenames;
    for (const auto &file : files) {
        writeXML(file.first, workdir.filePath(file.second));
        filenames.append(workdir.filePath(file.second));
    }

    QStringList args = { "-o", QString("%1.fzpz").arg(prefix) };
    args.append(filenames);
    int result = QProcess::execute(settings.value("minizip", "minizip").toString(), args);
    qDebug() << "minizip:" << result;
    if (result)
        throw std::runtime_error("failed to execute minizip");

    showPartPreviews(breadboard, schematic, pcb);

}

void MainWindow::showPartPreviews (const Part &part) {
    QDomDocument pcb = generatePCB(part);
    QDomDocument breadboard = generateBreadboard(part);
    QDomDocument schematic = generateSchematic(part);
    showPartPreviews(breadboard, schematic, pcb);
}

void MainWindow::showPartPreviews(const QDomDocument &bb, const QDomDocument &sc, const QDomDocument &pcb) {
    ui->svgPCB->load(pcb.toByteArray());
    ui->svgPCB->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->svgBreadboard->load(bb.toByteArray());
    ui->svgBreadboard->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->svgSchematic->load(sc.toByteArray());
    ui->svgSchematic->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
}

void MainWindow::on_actLocateMinizip_triggered()
{
    QString minizip = settings.value("minizip").toString();
    minizip = QFileDialog::getOpenFileName(this, "Locate minizip", minizip);
    if (minizip == "")
        return;
    minizip = QFileInfo(minizip).absoluteFilePath();
    settings.setValue("minizip", minizip);
}
