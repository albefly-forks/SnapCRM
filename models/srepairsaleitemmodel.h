#ifndef SREPAIRSALEITEMMODEL_H
#define SREPAIRSALEITEMMODEL_H

#include "scomrecord.h"
#include <QObject>
#include <QSqlRecord>
#include "models/sstoreitemmodel.h"
#include "widgets/shortlivednotification.h"

class SRepairSaleItemModel : public SComRecord
{
    Q_OBJECT
public:
    enum State {Requested = 0, EngineerBasket = 1, RepairLinked = 2, Sold = 3, Archive = 4};
    explicit SRepairSaleItemModel(QObject *parent = nullptr);
    explicit SRepairSaleItemModel(const QList<QStandardItem *> &record, QObject *parent = nullptr);
    ~SRepairSaleItemModel();
    int id();
    void load(const int);
    int itemId();
    void setItemId(const int);
    QString name();
    void setName(const QString);
    int count();
    void setCount(const int);
    void setCreated(const QDateTime);
    int fromUser();
    void setFromUser(const int);
    int toUser();
    void setToUser(const int);
    QString notes();
    void setNotes(const QString);
    int state();
    void setState(const int);
    int repairId();
    void setRepairId(const int);
    int workId();
    void setWorkId(const int);
    float price();
    void setPrice(const float);
    QString sn();
    void setSn(const QString);
    int warranty();
    void setWarranty(const int);
    bool rLock();
    void setRLock(const bool);
    bool sale();
    bool reserve();
    bool unsale();
    bool free();
    void setClient(const int);
    void setUnsaleReason(const QString&);
    QSqlRecord* actualStoreQtys();
    bool isProfitable();
    bool integrityStatus();
private:
    SStoreItemModel *m_storeItem = nullptr;
    bool nIntegrityErr = 1;
    int m_itemId;
    QString m_name;
    int m_count;
    int m_fromUser;
    int m_toUser;
    QString m_notes;
    int m_state;
    int m_repairId;
    int m_workId;
    float m_price;
    QString m_sn;
    int m_warranty;
    bool m_rLock;
    int m_customer = 0;
    bool m_isRealization = 0;
    float m_returnPercent = 0;
    float m_inPrice = 0;
    QString m_unsaleReason;
    int m_dealer;
    int m_buyer;
    bool commit();
    bool checkAvailabilityBefore(QSqlRecord*);
    bool updateStoreItemsTable();
    bool verifyQty(QSqlRecord*, QSqlRecord*);
    void showNotification(const QString&);
    bool dealerRoyalty();

};

#endif // SREPAIRSALEITEMMODEL_H