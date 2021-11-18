#include "loginwindow.h"
#include "mainwindow.h"
#include "my_widget.h"
#include <QApplication>
#define NO_LOGIN

void InitDBConnectionClass();

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	LoginWindow windowLogin;
	MainWindow windowMain;

//    MyWidget mw1{new MyWidget};
//    mw1.setWindowTitle("ctor: MyWidget(MyWidget*)");
//    mw1.show();

//    MyMainWindow mmw1{&mw1};
//    mmw1.setWindowTitle("ctor: MyMainWindow(MyWidget*)");
//    mmw1.show();

//    MyMainWindow mmw2{&mmw1};
//    mmw2.setWindowTitle("ctor: MyMainWindow(MyMainWindow*)");
//    mmw2.show();

//    MyWidget mw2{&mmw2};
//    mw2.setWindowTitle("ctor: MyWidget(MyMainWindow*)");
//    mw2.show();


    QObject::connect(&windowLogin,SIGNAL(ConnectToDB(QString,QString,uchar)),&windowMain,SLOT(ConnectToDB(QString,QString,uchar)));
	QObject::connect(&windowMain,SIGNAL(DBConnectErr(QString)),&windowLogin,SLOT(DBConnectErr(QString)));
	QObject::connect(&windowMain,SIGNAL(DBConnectOK()),&windowMain,SLOT(show()));
	QObject::connect(&windowMain,SIGNAL(DBConnectOK()),&windowLogin,SLOT(close()));
	QObject::connect(&windowLogin,SIGNAL(btnCancelClick()),&app,SLOT(quit()));



#ifdef NO_LOGIN
	windowMain.show();
#elif
	windowLogin.show();
#endif

	return app.exec();
}
