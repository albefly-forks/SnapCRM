#include "global.h"
#include "appver.h"
#include "windowsdispatcher.h"
#include "loginwindow.h"
#include "mainwindow.h" // подключать файл нужно именно здесь, по другому компилятор ругается
#include "chooseofficewindow.h"
#include "models/sofficemodel.h"

windowsDispatcher::windowsDispatcher(QObject *parent) :
    QObject(parent)
{
    setObjectName("windowsDispatcherObj");

    debugInitSettings();
    appLog->appendRecord(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " Application start");
    LoginWindow *windowLogin = new LoginWindow(this);

    QObject::connect(windowLogin,SIGNAL(DBConnectOK()),this,SLOT(connectOK()));
    QObject::connect(windowLogin,SIGNAL(btnCancelClick()),this,SIGNAL(quit()));

    windowLogin->show();
}

windowsDispatcher::~windowsDispatcher()
{
}

void windowsDispatcher::debugInitSettings()
{
    QFile file;
    QDir appDataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));   // C:/Users/user/AppData/Local
    file.setFileName(SLocalSettings::appSettingsPath()+"/../debug.ini");

    if (file.exists())
    {
        debugOptions = new  QSettings(file.fileName(), QSettings::IniFormat);
        debugOptions->setProperty("fileName", file.fileName());
    }
}

void windowsDispatcher::connectOK()
{
    initSystemObjects();
    initUserDbData();
    initPermissions();
    initCompanies();
    initOffices();

    QFont f;
    f.setFamily(userLocalData->FontFamily.value);
    f.setPixelSize(userDbData->fontSize);
    QApplication::setFont(f);

    userDbData->updateLoginTimestamp();
    userActivityLog->appendRecord(tr("Login"));   // Заменено на "Login", потому что АСЦ не позволяет запускать два экз. программы, а определение происходит по фразе "Выполнен вход в систему"

    userDbData->currentOffice = userDbData->office;

    if ((permissions->changeOffice && (officesModel->rowCount() > 1)) || SOfficeModel::current()->id() == 0)
    {   // АСЦ успешно логинится с архивным офисом в настройках сотрудника, это неправильно.
        createChooseOfficeWindow();
    }
    else
    {
        createMainWindow();
    }

    if(debugLoginOptions)
        delete debugLoginOptions;
}

void windowsDispatcher::createChooseOfficeWindow()
{
    if(debugLoginOptions)
        if(debugLoginOptions->contains("office"))
        {
            userDbData->currentOffice = debugLoginOptions->value("office").toInt();
            if(SOfficeModel::current()->id())
            {
                createMainWindow();
                return;
            }
        }

    chooseOfficeWindow *windowChooseOffice = new chooseOfficeWindow(this);
    QObject::connect(windowChooseOffice, SIGNAL(officeChoosed()), this, SLOT(createMainWindow()));
    windowChooseOffice->show();
}

void windowsDispatcher::createMainWindow()
{
    userDbData->company = SOfficeModel::current()->defaultCompany();
    MainWindow *windowMain = MainWindow::getInstance(this); // указатель должен объявляться именно здесь, по другому компилятор ругается
    windowMain->show();
    windowMain->createTabRepairs(); // по-умолчанию создаём вкладку Ремонты
}

