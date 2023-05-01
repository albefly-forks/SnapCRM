#include "global.h"
#include "stablesalarypaymentsmodel.h"

STableSalaryPaymentsModel::STableSalaryPaymentsModel(QObject *parent) : STableBaseModel(parent)
{

}

QVariant STableSalaryPaymentsModel::data(const QModelIndex &item, int role) const
{
    if(role == Qt::DisplayRole)
    {
        switch (item.column())
        {
            case 1: return timestampLocal(item);
            case 2: return dataLocalizedFromDouble(item);
            default: return STableBaseModel::data(item, role);
        }
    }
    return STableBaseModel::data(item, role);
}