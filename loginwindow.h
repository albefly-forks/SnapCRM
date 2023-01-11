#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QtXml>
#include <QDomDocument>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QResource>
#include <QIODevice>
#include "widgets/shortlivednotification.h"
#include "windowsdispatcher.h"
#include "models/ssloptionsdialog.h"
#define ASC_SETTINGS_PATH   "VLab/ASC.exe_Url_1ttwwpflaim4gbzvmtfpiarttwuebugu"    // путь к файлу настроек АЦС для пути к приложению "C:\Program Files (x86)\ASCApp\ASC.exe"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QWidget
{
	Q_OBJECT

signals:
	void btnCancelClick();
    void DBConnectOK();

public:
    explicit LoginWindow(QObject *parent = nullptr);
	~LoginWindow();
    void debugLogin();

private:
	Ui::LoginWindow *ui;
    QGraphicsPixmapItem *logo;
    QGraphicsScene *logoScene;
    QSqlDatabase connMain;
    QSqlDatabase connNtfy;
    QSqlDatabase connThird;
//    QSqlDatabase connFourth;
    bool selMostRecentSettingFile(QDir &);
    bool genSettingsFilePath();
    bool genAscSettingsFilePath();
    bool readSettings(QFile &file);
    bool readSettingsNode(QDomNode &n);
    bool saveSettings();
    QDomDocument *settingsDOM = nullptr;
    QDomElement docElem;
    QFile settingsFile;
    QFile settingsAscFile;
    QDir settingsPath;
    QDir settingsAscPath;
    QFile prevAppVerSettingsFile;
    void statusBarMsg(const QString&, int delay = 2500);
    QTimer *statusBarDelay;
    SSLOptionsDialog *modalWidget;
    QWidget *overlay;
    bool checkAppVer();
    int checkSchema();
    QStringList usersOnline();
    bool updateDB(int);
    void closeConnections();
    void debugInitLoginOptions();

private slots:
    void editPassword_onReturnPressed();
    void btnLoginHandler();
    void btnCancelHandler();
    void clearStatusLabel();
    void createSSLOptionsDialog();
    void closeSSLOptionsDialog();
};

#endif // LOGINWINDOW_H
