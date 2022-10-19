#ifndef TABREPAIR_H
#define TABREPAIR_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QToolButton>
#include <QStyle>
#include <QTableWidget>
#include <QLabel>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlField>
#include <QScrollBar>
#include <QTimeZone>
#include <QDateTime>
#include <QLocale>
#include "tabcommon.h"
#include "widgets/getoutdialog.h"
#include "widgets/sgroupboxeventfilter.h"
#include "models/srepairmodel.h"
#include "models/sfieldsmodel.h"
#include "models/scommentmodel.h"
#include "models/scommentsmodel.h"

namespace Ui {
class tabRepair;
}

class commentsTable : public QTableView
{
    Q_OBJECT

public:
    explicit commentsTable(QWidget *parent = nullptr);
    ~commentsTable();
    void resizeEvent(QResizeEvent*);
#if QT_VERSION >= 0x060000
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>());
#else
    void dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &roles = QVector<int>());
#endif
private:
};

class commentsDataModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    explicit commentsDataModel(QWidget *parent = nullptr);
    ~commentsDataModel();
    void update();
private:
    QVariant data(const QModelIndex &index, int role) const;
};

class worksAndSparePartsTable : public QTableView
{
    Q_OBJECT

public:
    explicit worksAndSparePartsTable(QWidget *parent = nullptr);
    ~worksAndSparePartsTable();
    void resizeEvent(QResizeEvent*);
#if QT_VERSION >= 0x060000
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>());
#else
    void dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &roles = QVector<int>());
#endif
private:
};

class worksAndSparePartsDataModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    enum {actions, name, qty, price, summ, warranty, user, sn, id, work_id, item_rsrv_id};
    explicit worksAndSparePartsDataModel(QWidget *parent = nullptr);
    ~worksAndSparePartsDataModel();
    void update();
private:
    QVariant data(const QModelIndex &index, int role) const;
};

class tabRepair : public tabCommon
{
    Q_OBJECT

    friend class getOutDialog;

signals:
    void worksTreeDoubleClicked(int);
    void createTabPrevRepair(int);
    void generatePrintout(QMap<QString,QVariant>);
    void createTabClient(int);

public:
    explicit tabRepair(int rep_id, MainWindow *parent = nullptr);
    static tabRepair* getInstance(int rep_id, MainWindow *parent = nullptr);
    ~tabRepair();
    QString tabTitle() override;
private:
    Ui::tabRepair *ui;
    static QMap<int, tabRepair*> p_instance;
    int repair_id;
    SRepairModel *repairModel;
    SClientModel *clientModel;
    SFieldsModel *additionalFieldsModel;
    QSortFilterProxyModel *statusesProxyModel;
    commentsDataModel *commentsModel;
    worksAndSparePartsDataModel *worksAndPartsModel;
    bool statusUpdateFlag;
    float works_sum, parts_sum, total_sum;
    int getFieldIdByName(const QString &, QSqlQueryModel *);
    QString getDisplayRoleById(int, QAbstractItemModel*, int column = 0);
    void eventResize(QResizeEvent *);
    void addItemToListViewExtraInfo(QString, QString);
    void setLock(bool);
    QString box_name;
    bool modelRO = 0;   // признак блокировки карты ремонта
    getOutDialog *modalWidget;
    QWidget *overlay;
    bool save_state_on_close = 0;
    bool worksAndPartsEditEnabled = 0;
    SGroupBoxEventFilter *groupBoxEventFilter;
    int m_clientId = 0;
    QList<QWidget*> additionalFieldsWidgets;
    void createAdditionalFieldsWidgets();
    void delAdditionalFieldsWidgets();
#ifdef QT_DEBUG
    void randomFill(){};
#endif

private slots:
    void reloadRepairData();
    void updateWidgets();
    void worksTreeDoubleClicked(QModelIndex);
    void saveStatus();
    void saveStatus(int);
    void comboBoxIndexChanged(int);
    void updateTotalSumms();
    void createGetOutDialog();
    void closeGetOutDialog();
    void openPrevRepair();
    void printStickers(int);
    void changeOffice(int);
    void changeManager(int);
    void changeEngineer(int);
    void openInvoice(int);
    void quickAddSparePartByUID(int);
    void editIncomingSet(int);
    void setAgreedAmount(int);
    void buttonClientClicked();
#ifdef QT_DEBUG
    void test_scheduler_handler(){};
    void test_scheduler2_handler(){};
#endif
};

#endif // TABREPAIR_H
