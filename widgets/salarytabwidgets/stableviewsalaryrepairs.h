#ifndef STABLEVIEWSALARYREPAIRS_H
#define STABLEVIEWSALARYREPAIRS_H

#include "../stableviewbase.h"
#include "widgets/saletableitemdelegates.h"

class STableViewSalaryRepairs : public STableViewBase
{
    Q_OBJECT
signals:
    void signalColumnResized(int column, int newWidth);
public:
    explicit STableViewSalaryRepairs(QWidget *parent = nullptr);
    ~STableViewSalaryRepairs();
    XtraSerializer *gridLayout();
protected slots:
    void columnResized(int column, int oldWidth, int newWidth);
};

class STableViewSalaryRepairsItemDelegates : public STableViewBaseItemDelegates
{
    Q_OBJECT
public:
    explicit STableViewSalaryRepairsItemDelegates(QWidget *parent = nullptr);

};

#endif // STABLEVIEWSALARYREPAIRS_H