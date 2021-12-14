#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QDialog>

namespace Ui {
class HelpWindow;
}

class HelpWindow : public QDialog {
    Q_OBJECT
public:
    explicit HelpWindow(QWidget *parent = nullptr);
    ~HelpWindow();
    void setHelpFont (const QFont &font, float sizemult);
public slots:
    void display ();
private:
    Ui::HelpWindow *ui;
};

#endif // HELPWINDOW_H
