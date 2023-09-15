#ifndef SLINEEDIT_H
#define SLINEEDIT_H

#include <QLineEdit>
#include <QStringList>
#include <QToolButton>
#include <QStyle>
#include <QVector>
#include <QSignalMapper>
#include <QMetaEnum>
#include <QDebug>

class SLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString buttons READ buttons WRITE setButtons)

signals:
    void buttonClicked(int buttonId = 0);
    void mouseDoubleClick();
    void keyPress(QKeyEvent *event);

public:
    enum Buttons {Clear, DownArrow, Edit, Search, Print, Apply, Open};
    SLineEdit(QWidget *parent = nullptr);
    void setButtons(QString buttons);
    void clearButtons();
    void arrangeButtons();
    void resize(const QSize &);
    void resize(int, int);
    QString buttons();
    void tmp_set_buttons_style_sheet(const QString&);
    void enableExtKeyPressHandler(bool);
    ~SLineEdit();
    Q_ENUM(Buttons);

private:
    QStringList allowedButtonsList = { "Clear", "DownArrow", "Edit", "Search", "Print", "Apply", "Open" };
//    QList<QString> buttonIconList = { "🗙", "▼", "🖉", "🔍", "🖶", "🗸", "🗁" };
    // другие варианты unicode иконок кнопок: ✖ или 🗙 или ⌫, ▼, 🖉, 🔍, 🖶 или 🖨, 🗸, 🗁 или 📂
    QList<QString> buttonIconList = { "1F5D9_32.png", "25BC_32.png", "1F589_32.png", "1F50D_32.png", "1F5A8_32.png", "1F5F8_32.png", "1F4C2_32.png" };

    QStringList *buttonsList;
    int buttonsCount = 0;
    QVector<QToolButton*> lineEditButtons;
    QSignalMapper *signalMapper;
    int frameWidth;
    QSize sz;
    QSize buttonSize;
    bool m_extKeyPressHandler = 0;

protected:
    void resizeEvent(QResizeEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
};


#endif // SLINEEDIT_H
