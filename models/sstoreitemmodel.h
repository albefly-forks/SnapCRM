#ifndef SSTOREITEMMODEL_H
#define SSTOREITEMMODEL_H

#include "scomrecord.h"
#include <QObject>
#include "models/sclientmodel.h"

class SStoreItemModel : public SComRecord
{
    Q_OBJECT
public:
    enum OpOnItem { Nop = 0, Sale = 1, Reserve = 2, SaleReserved = 3, SaleRepair = 4, Unsale = 5, Free = 6, WriteOff = 7, CancelWriteOff = 8, UnsaleRepair = 9, ReserveRepair = 10 };
    enum QtyField { Count = 0, Sold = 1, Reserved = 2 };
    enum SaleMode {Store = 1, Repair = 2};
    enum SaleOpColumns {ColId = 0, ColUID, ColName, ColCount, ColAvail, ColPrice, ColSumm, ColBox, ColSN, ColWarranty, ColUser, ColRealization, ColRetPercent, ColState, ColNotes, ColItemId, ColInPrice, ColObjId, ColDealer, ColBuyer, ColCreated, ColWorkId, ColRecordType, ColWorkType};
    Q_ENUM(SaleOpColumns)
    explicit SStoreItemModel(QObject *parent = nullptr);
    explicit SStoreItemModel(const QList<QStandardItem *> &record, const int qtyToSale = 0, QObject *parent = nullptr);
    ~SStoreItemModel();
    int id();
    bool isHidden();
    void setHidden(const bool);
    int articul();
    void setArticul(const int);
    int dealer();
    void setDealer(const int);
    bool isRealization();
    void setRealization(const bool);
    int isDealerLock();
    void setDealerLock(const int);
    QString name();
    void setName(const QString&);
    int state();
    void setState(const int);
    int category();
    void setCategory(const int);
    int store();
    void setStore(const int);
    QDateTime updated();
    void setUpdated(const QDateTime);
    int count();
    void setCount(const int);
    int reserved();
    void setReserved(const int);
    int units();
    void setUnits(const int);
    int box();
    void setBox(const int);
    QString boxName();
    void setBoxName(const QString&);
    int priceOption();
    void setPriceOption(const int);
    double currencyRate();
    void setCurrencyRate(const double);
    double inPrice();
    void setInPrice(const double);
    double price();
    void setPrice(const double);
    double price2();
    void setPrice2(const double);
    double price3();
    void setPrice3(const double);
    double price4();
    void setPrice4(const double);
    double price5();
    void setPrice5(const double);
    int document();
    void setDocument(const int);
    int partRequest();
    void setPartRequest(const int);
    QString shopTitle();
    void setShopTitle(const QString&);
    QString shopDescription();
    void setShopDescription(const QString&);
    QString SN();
    void setSN(const QString&);
    QString PN();
    void setPN(const QString&);
    int description();
    void setDescription(const int);
    int isShopEnable();
    void setShopEnable(const int);
    QString intBarcode();
    void setIntBarcode(const QString&);
    int extBarcode();
    void setExtBarcode(const int);
    int inCount();
    void setInCount(const int);
    double inSumm();
    void setInSumm(const double);
    QString notes();
    void setNotes(const QString&);
//    int img1();
//    void setImg1(const int);
//    int img2();
//    void setImg2(const int);
//    int img3();
//    void setImg3(const int);
//    int img4();
//    void setImg4(const int);
//    int img5();
//    void setImg5(const int);
    int minimumInStock();
    void setMinimumInStock(const int);
    int sold();
    void setSold(const int);
    int returnPercent();
    void setReturnPercent(const int);
    int warranty();
    void setWarranty(const int);
    int warrantyDealer();
    void setWarrantyDealer(const int);
    bool isNotForSale();
    void setNotForSale(const bool);
    int stState();
    void setStState(const int);
    QString stNotes();
    void setStNotes(const QString&);
    bool isGeHighlight();
    void setGeHighlight(const bool);
//    QDate lastStocktaking_date();
//    void setLastStocktakingDate(const QDate);
    void load();
    void setSaleMode(SaleMode mode);
    QMap<int, int> *loadQtys();
    void delIntegrityCheckObjects();
    bool checkBeforeSale();
    bool checkAfterSale();
    bool sale(const QString &logText);
    bool reserve(const QString &logText);
    bool saleReserved(const QString &logText);
    bool saleStore(const QString &logText);
    bool saleRepair(const QString &logText);
    bool free(const QString &logText);
    bool unsale(const QString &logText);
    bool unsaleRepair(const QString &logText);
    bool writeOff(const QString &logText);
    bool cancelWriteOff(const QString &logText);
    bool dealerRoyalty(const SBalanceLogRecordModel::RoyaltyReason);
    void showNotification(const QString&);
    bool commit();
private:
    bool m_isHidden;
    int m_articul;
    int m_dealer;
    bool m_isRealization;
    int m_isDealerLock;
    QString m_name;
    int m_state;
    int m_category;
    int m_store;
    QDateTime m_updated;
    int m_count;
    int m_reserved;
    int m_units;
    int m_box;
    QString m_boxName;
    int m_priceOption;
    double m_currencyRate;
    double m_inPrice;
    double m_price;
    double m_price2;
    double m_price3;
    double m_price4;
    double m_price5;
    int m_document;
    int m_partRequest;
    QString m_shopTitle;
    QString m_shopDescription;
    QString m_SN;
    QString m_PN;
    int m_description;
    int m_isShopEnable;
    QString m_intBarcode;
    int m_extBarcode;
    int m_inCount;
    double m_inSumm;
    QString m_notes;
//    int m_img1;
//    int m_img2;
//    int m_img3;
//    int m_img4;
//    int m_img5;
    int m_minimumInStock;
    int m_sold;
    int m_returnPercent;
    int m_warranty;
    int m_warrantyDealer;
    bool m_isNotForSale;
    int m_stState;
    QString m_stNotes;
    bool m_isGeHighlight;
//    QDate m_lastStocktaking_date;
    int m_saleId;
    int m_savedSaleQty;
    QMap<int, int> *m_qtysBeforeSale = nullptr;
    QMap<int, int> *m_qtysAfterSale = nullptr;
    int m_saleMode;
    int m_opOnItemType;
    SBalanceLogRecordModel::RoyaltyReason m_balUpdReason;
    int m_saleObjId;
    int m_saleQty;
    int m_buyerId;
    QString m_unsaleReason;
    int m_saleUser;
};

#endif // SSTOREITEMMODEL_H
