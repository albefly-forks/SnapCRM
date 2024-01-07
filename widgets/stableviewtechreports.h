#ifndef STABLEVIEWTECHREPORTS_H
#define STABLEVIEWTECHREPORTS_H

#include "stableviewbase.h"
#include <QObject>

class STableViewTechReports : public STableViewBase
{
    Q_OBJECT
public:
    enum Column{ID = 0, Num, Created, User, Company, Client, Device, InvNumber, SerialNumber, ProductionData, PurchaseDate, InitCost, ResidualCost, Fault, DiagResult, Conclusion, Notes};
    Q_ENUM(Column)
    enum ColumnWidth{ID_w = 40, Num_w = 40, Created_w = 120, User_w = 110, Company_w = 110, Client_w = 150, Device_w = 120, InvNumber_w = 80, SerialNumber_w = 80, ProductionData_w = 80, PurchaseDate_w = 80, InitCost_w = 60, ResidualCost_w = 60, Fault_w = 120, DiagResult_w = 120, Conclusion_w = 120, Notes_w = 120};
    Q_ENUM(ColumnWidth)
    explicit STableViewTechReports(QWidget *parent = nullptr);
    QList<int> *selectedReportsList();
private:
    void translateNames();
};

#endif // STABLEVIEWTECHREPORTS_H
