#include "helpwindow.h"
#include "ui_helpwindow.h"
#include <QResource>
#include <QDomDocument>

HelpWindow::HelpWindow(QWidget *parent) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::HelpWindow)
{
    ui->setupUi(this);

    auto content = ui->content;
    content->setMarkdown(QString::fromLatin1(QResource(":/help/manual").uncompressedData()));

    // === begin nightmare of forcing qt5 to style markdown sanely ===

    QDomDocument doc;
    doc.setContent(content->toHtml());

    QStringList keepers = { "font-family", "font-weight", "font-style", "vertical-align" };
    for (QList<QDomElement> els = { doc.documentElement() }; !els.empty(); ) {
        QDomElement el = els.front();
        els.pop_front();
        // all of these make things get weird. remove them.
        if (el.tagName() == "br" || el.tagName() == "style" || el.tagName() == "meta") {
            el.parentNode().removeChild(el);
            continue;
        }
        // traverse.
        for (auto ec = el.firstChildElement(); !ec.isNull(); ec = ec.nextSiblingElement())
            els.append(ec);
        // discard unwanted style attribute values.
        QStringList sts = el.attribute("style").split(";"), stk;
        for (auto st : sts)
            if (keepers.contains(st.split(":")[0].trimmed()))
                stk.append(st.trimmed());
        if (stk.empty() || el.tagName() == "body")
            el.removeAttribute("style");
        else
            el.setAttribute("style", stk.join("; "));
        // dom manipulations to make code blocks look less shit.
        if (el.tagName() == "span" &&
                el.attribute("style").contains("font-family") &&
                el.parentNode().toElement().tagName() == "p" &&
                el.parentNode().toElement().text().trimmed() == el.text().trimmed())
            el.parentNode().toElement().setAttribute("class", "code");
        if (el.tagName() == "p" &&
                el.attribute("style").contains("font-family") &&
                el.text() == "")
        {
            el.setAttribute("class", "code");
            el.appendChild(doc.createEntityReference("nbsp"));
        }
    }

    // right.
    content->document()->setDefaultStyleSheet(QString::fromLatin1(QResource(":/help/manual.css").uncompressedData()));
    content->setHtml(doc.toString());

    // === end nightmare (sort of) ===

}

HelpWindow::~HelpWindow()
{
    delete ui;
}

void HelpWindow::setHelpFont(const QFont &font, float sizemult) {
    QFont f = font;
    f.setPointSizeF(font.pointSizeF() * sizemult);
    setFont(f);
    // seems to look nice i guess:
    resize(150 * fontMetrics().averageCharWidth(), 40 * fontMetrics().height());
}

void HelpWindow::display () {
    showNormal();
    activateWindow();
    raise();
}
