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
#include <QClipboard>
#include "tabcommon.h"
#include "widgets/getoutdialog.h"
#include "widgets/sgroupboxeventfilter.h"
#include "models/srepairmodel.h"
#include "models/srepairstatuslog.h"
#include "models/sfieldsmodel.h"
#include "models/ssortfilterproxymodel.h"
#include "models/ssaletablemodel.h"
#include "widgets/saletableitemdelegates.h"

namespace Ui {
class tabRepair;
}

class worksAndSparePartsTable : public QTableView
{
    Q_OBJECT

signals:
    void createTabSparePart(int);
    void createTabSparePartReserve(int);
public:
    explicit worksAndSparePartsTable(QWidget *parent = nullptr);
    ~worksAndSparePartsTable();
    void resizeEvent(QResizeEvent*) override;
    void setModel(QAbstractItemModel *model) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
protected:
    int sizeHintForColumn(int column) const override;
private:
    SSaleTableModel *m_model = nullptr;
    int m_modelRowCount = 0;
    int colWidths[12] = {0,60,90,0,45,40,70,70,120,120,80,100}; // 0-й эл-т — пустышка
    QFontMetrics *fontMetrics;
    void resizeColumnToContents(int column);
    void resizeColumnsToContents();
    void findWidthByContents(const int column);
protected slots:
    void columnResized(int column, int oldWidth, int newWidth);
public slots:
    void reset() override;
private slots:
#if QT_VERSION >= 0x060000
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>()) override;
#else
    void dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &roles = QVector<int>()) override;
#endif
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
    void createTabSparePart(int);
    void createTabSparePartReserve(int);
    void createTabSelectItem(int, QWidget*);

public:
    explicit tabRepair(int rep_id, MainWindow *parent = nullptr);
    static tabRepair* getInstance(int rep_id, MainWindow *parent = nullptr);
    ~tabRepair();
    QString tabTitle() override;
    bool tabCloseRequest() override;
private:
    QSqlQuery *query;
    bool nErr = 1;
    Ui::tabRepair *ui;
    static QMap<int, tabRepair*> p_instance;
    int repair_id;
    SRepairModel *repairModel;
    SClientModel *clientModel;
    SFieldsModel *additionalFieldsModel;
    SSortFilterProxyModel *statusesProxyModel;
    SSaleTableModel *worksAndPartsModel;
    SaleTableItemDelegates *itemDelagates;
    bool m_statusUpdateInProgress = 0;
    double works_sum, parts_sum, total_sum;
    QString box_name;
    bool modelRO = 0;   // признак блокировки карты ремонта
    getOutDialog *modalWidget;
    QWidget *overlay;
    bool save_state_on_close = 0;
    bool m_worksRO = 1;
    bool m_diagRO = 1;
    bool m_summRO = 1;
    bool m_getOutButtonVisible = 0;
    bool m_comboBoxStateEnabled = 1;
    bool m_comboBoxNotifyStatusEnabled = 1;
    bool m_outDateVisible = 0;
    SGroupBoxEventFilter *groupBoxEventFilter;
    int m_clientId = 0;
    QList<QWidget*> additionalFieldsWidgets;
    bool m_autosaveDiag = 0;
    QTimer *m_autosaveDiagTimer;
    bool m_diagChanged = 0;
    bool m_spinBoxAmountChanged = 0;
    bool m_groupUpdate = 0;
    SRepairStatusLog *repairStatusLog;
    bool m_buttonSaveStateEnabled = 0;
    int getFieldIdByName(const QString &, QSqlQueryModel *);
    QString getDisplayRoleById(int, QAbstractItemModel*, int column = 0);
    void eventResize(QResizeEvent *);
    void fillExtraInfo();
    void setLock(bool);
    void createAdditionalFieldsWidgets();
    void delAdditionalFieldsWidgets();
    void setInfoWidgetVisible(QWidget *w, bool state = 1);
    bool setWidgetsParams(const int);
    bool checkStateAcl(const int);
    bool checkData(const int);
    void updateStatesModel(const int statusId);
    void doStateActions(const int);
    void setPricesToZero();
    bool commit(const QString &notificationCaption = tr("Успешно"), const QString &notificationText = tr("Данные сохранены"));
#ifdef QT_DEBUG
    void randomFill() override {};
#endif

private slots:
    void reloadRepairData();
    void updateWidgets();
    void tableRowDoubleClick(QModelIndex);
    void saveState();
    void saveState(int);
    void comboBoxStateIndexChanged(int);
    void updateTotalSumms(const double, const double, const double);
    void saveTotalSumms();
    void createGetOutDialog();
    void closeGetOutDialog();
    void openPrevRepair();
    void printStickers(int);
    void changeOffice(int);
    void changeManager(int);
    void changeEngineer(int);
    void openInvoice(int);
    void onReturnQuickAddPart();
    bool quickAddPart(const int);
    void editIncomingSet(int);
    void setAgreedAmount(int);
    void buttonClientClicked();
    void setInformedStatus(int);
    void diagChanged();
    void diagEditFinished();
    void spinBoxAmountChanged(double);
    void spinBoxAmountEditingFinished();
    void saveDiagAmount();
    void diagAmountSaved();
    void buttonAddItemClicked();
    void switchEditStrategy(bool);
    void saveSaleTableClicked();
    void setSaveSaleTableEnabled();
    void buttonWorksAdminEdit(bool state);
    void addCustomWork();
#ifdef QT_DEBUG
    void test_scheduler_handler() override{};
    void test_scheduler2_handler() override{};
#endif
};

#endif // TABREPAIR_H
