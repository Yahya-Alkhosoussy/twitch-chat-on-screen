#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

namespace Ui {
class preferences;
}

class preferences : public QDialog
{
    Q_OBJECT

public:
    explicit preferences(QWidget *parent = nullptr);
    ~preferences();

private:
    Ui::preferences *ui;
};

#endif // PREFERENCES_H
