#ifndef SMODALWIDGET_H
#define SMODALWIDGET_H

#include "widgets/swidget.h"
#include "global.h"
#include "shortlivednotification.h"
#include "mainwindow.h"

class SModalWidget : public SWidget
{
    Q_OBJECT
signals:
    void onDelete();
public:
    enum State {Visible = 0, Hidden = 1};
    explicit SModalWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    explicit SModalWidget(QWidget *parent = nullptr, State state = Visible);
    ~SModalWidget();
private:
    QWidget *background = nullptr;
};

#endif // SMODALWIDGET_H
