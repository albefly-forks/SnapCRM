#ifndef SDIALOGISSUEREPAIR_H
#define SDIALOGISSUEREPAIR_H

#include "widgets/smodalwidget.h"
#include <QLocale>
#include "global.h"
#include "shortlivednotification.h"
#include "models/srepairmodel.h"
#include "models/sclientmodel.h"
#include "models/scashregistermodel.h"
#include "models/ssaletablemodel.h"
#include "models/ssortfilterproxymodel.h"
#include "models/sworkshopissuedmodel.h"
#include "models/srepairstatuslog.h"
#include "reports/sprintposreport.h"

namespace Ui {
class SDialogIssueRepair;
}

class SDialogIssueRepair : public SModalWidget
{
    Q_OBJECT

signals:
    void issueSuccessfull();
    void issueFailed();
public:
    explicit SDialogIssueRepair(QList<SRepairModel*> repairs, Qt::WindowFlags flags = Qt::WindowFlags(), QWidget *parent = nullptr);
    ~SDialogIssueRepair();
    void setRepairModel(SRepairModel *model);
    void setClientModel(SClientModel *model);
    void setSaleTableModel(SSaleTableModel *model);
    void deleteRepairModels();
private:
    Ui::SDialogIssueRepair *ui;
    bool m_summsNotEq = 0;
    bool m_balanceNotEnough = 1;
    bool m_singleRepairWidgetsVisible = 1;
    bool m_rejectReasonWidgetsVisible;
    SClientModel *m_clientModel;
    SSortFilterProxyModel *m_paymentSystemsProxyModel;
    QList<SRepairModel*> m_repairsModels;
    bool m_pushButtonIssueEnabled = 1;
    double m_totalPrepayAmount = 0;
    double m_totalAgreedAmount = 0;
    double m_totalAmount = 0;
    double m_totalAmountToPay = 0;
    bool m_isCartridgeIssue = 0;
    void setDefaultStyleSheets();
    void initPaymentSystems();
    bool checkInput();
    bool checkAmounts();
    void collectRepairsData();
    void setRepairReady(SRepairModel *model);
    void issueRepairs();
    void printCartridgeWorksReport();
private slots:
    void buttonIssueClicked();
    void buttonCancelClicked();
    void createLooseDoc();
    void otheRejectReasonShow(QString);
    void textEditTextChanged();
    void paymentSystemChanged(int);
    void checkBoxInCreditToggled(bool);
};

#endif // SDIALOGISSUEREPAIR_H
