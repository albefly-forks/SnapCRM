#include "ssaletablemodel.h"

SSaleTableModel::SSaleTableModel(QObject *parent) :
    QStandardItemModel(parent),
    m_queryData(new QSqlQueryModel)
{
    m_itemsPendingRemoveList = new QMap<int, int>;
    m_worksPendingRemoveList = new QMap<int, int>;

    setHorizontalHeaderLabels();

    connect(m_queryData, SIGNAL(modelReset()), this, SLOT(sqlDataChanged()));
#if QT_VERSION >= 0x060000
    connect(this, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &)), this, SLOT(dataChangedHook(const QModelIndex&, const QModelIndex&, const QList<int> &)));
#else
    connect(this, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &)), this, SLOT(dataChangedHook(const QModelIndex&, const QModelIndex&, const QVector<int> &)));
#endif
}

SSaleTableModel::~SSaleTableModel()
{
    delete m_itemsPendingRemoveList;
    delete m_worksPendingRemoveList;
    delete m_queryData;
}

QVariant SSaleTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case SStoreItemModel::SaleOpColumns::ColPrice:
        case SStoreItemModel::SaleOpColumns::ColSumm: return sysLocale.toString(QStandardItemModel::data(index, role).toDouble(), 'f', 2);
        case SStoreItemModel::SaleOpColumns::ColBox: return itemBoxesModel->getDisplayRole(QStandardItemModel::data(index, role).toInt(), 1);
        case SStoreItemModel::SaleOpColumns::ColWarranty: return warrantyTermsModel->getDisplayRole(QStandardItemModel::data(index, role).toInt(), 1);
        case SStoreItemModel::SaleOpColumns::ColUser: return allUsersMap->value(QStandardItemModel::data(index, role).toInt());
        }
    }
    if ( role == Qt::BackgroundRole )
    {
        if(isRowMarkedRemove(index.row()))
            return QColor("light gray");
    }

    return QStandardItemModel::data(index, role);
}

Qt::ItemFlags SSaleTableModel::flags(const QModelIndex &index) const
{
    int condition;
    if( m_modelState == State::StoreNew || m_modelState == State::StoreReserved || m_modelState == State::WorkshopRW || m_modelState == State::WorkshopAdm )   // у вкладки будет дополнительный режим — правка резерва (в АСЦ такого вроде не было)
    {
        condition = this->index(index.row(), SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool() << 7 | m_tableMode << 6 | index.column();
        switch (condition)
        {
            case SSaleTableModel::RecordType::Work << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColName:
            case SSaleTableModel::RecordType::Work << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColPrice:
                if(!this->index(index.row(), SStoreItemModel::SaleOpColumns::ColUID).data().toString().isEmpty())
                    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::StoreSale << 6    | SStoreItemModel::SaleOpColumns::ColCount:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::StoreSale << 6    | SStoreItemModel::SaleOpColumns::ColPrice:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::StoreSale << 6    | SStoreItemModel::SaleOpColumns::ColSN:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::StoreSale << 6    | SStoreItemModel::SaleOpColumns::ColWarranty:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::StoreSale << 6    | SStoreItemModel::SaleOpColumns::ColUser:
            case SSaleTableModel::RecordType::Work << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColCount:
            case SSaleTableModel::RecordType::Work << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColWarranty:
            case SSaleTableModel::RecordType::Work << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColUser:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColSN:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColWarranty:
                return Qt::ItemIsEnabled | Qt::ItemIsEditable;
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColPrice:
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColUser:
                if(m_modelState == State::WorkshopAdm)
                    return Qt::ItemIsEnabled | Qt::ItemIsEditable;
            case SSaleTableModel::RecordType::Item << 7 | SSaleTableModel::WorkshopSale << 6 | SStoreItemModel::SaleOpColumns::ColCount:
                if(m_modelState == State::WorkshopAdm && permissions->contains("69"))   // Устанавливать детали со склада
                    return Qt::ItemIsEnabled | Qt::ItemIsEditable;
            default:
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
    }
    else
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
}

bool SSaleTableModel::insertRecord(int row, const QSqlRecord &record, const int recType)
{
    Q_ASSERT_X(record.count() == columnCount(), "insertRecord()", "different column count");
    bool ret = 1;
    QList<QStandardItem*> rowData;
    QStandardItem *item;
    int recordId = record.value(SStoreItemModel::SaleOpColumns::ColId).toInt();

#ifdef QT_DEBUG
    int qtyLimit;
    double price;
    if(record.value(SStoreItemModel::SaleOpColumns::ColAvail).toInt() > 5)
        qtyLimit = 5;
    else
        qtyLimit = record.value(SStoreItemModel::SaleOpColumns::ColAvail).toInt() + 1;

    int rand = QRandomGenerator::global()->bounded(qtyLimit);
#endif

    int i;
    for(i = 0; i < record.count(); i++)
    {
        item = new QStandardItem();
        item->setData(record.value(i), Qt::EditRole);

        if( recordId == 0 || i == 0)   // все поля новых записей (добавленных в таблицу store_int_reserve впервые или для работ)
        {                              // или только 0-й столбец записи о товаре, добавленном из корзины сотрудника
            item->setData(record.value(SStoreItemModel::SaleOpColumns::ColRecordType).toInt(), DataRoles::RecordType);
            item->setData(1, DataRoles::Changed);
            if(recordId == 0 && i == SStoreItemModel::SaleOpColumns::ColUser)
            {
                item->setData(userDbData->value("id").toInt(), Qt::EditRole);
                // инициализация поля кодом пользователя производится тоже только для новых товаров; в будущем это позволит
                // реализовать режим админинстративной правки с возможностью вставлять детали из чужой корзины
            }
        }

#ifdef QT_DEBUG
        if(rand > 1 && i == SStoreItemModel::SaleOpColumns::ColCount)
            item->setData(rand, Qt::EditRole);
        if(rand > 1 && i == SStoreItemModel::SaleOpColumns::ColPrice)
            price = item->data(Qt::DisplayRole).toDouble();
        if(rand > 1 && i == SStoreItemModel::SaleOpColumns::ColSumm)
            item->setData(rand*price, Qt::EditRole);
#endif

        rowData << item;
    }

    QStandardItemModel::insertRow(row, rowData);
    emit amountChanged(amountTotal(), m_amountItems, m_amountWorks);

    ret = repair_autoSaveTables();
    if(!ret)
        QStandardItemModel::removeRow(row);

    return ret;
}

/* Возвращает 0 в случае неудачи
*/
bool SSaleTableModel::appendRecord(const QSqlRecord &record)
{
    return insertRecord(rowCount(), record);
}

int SSaleTableModel::isItemAlreadyInList(int id)
{
    for(int i = 0; i < rowCount(); i++)
    {
        if(value(i, SStoreItemModel::SaleOpColumns::ColItemId).toInt() == id)
            return i;
    }
    return -1;
}

/* Добавление произвольной работы в конец таблицы
*/
void SSaleTableModel::addCustomWork()
{
    qDebug().nospace() << "[" << this << "] addCustomWork()";
    QSqlRecord *customWork = new QSqlRecord();
    QSqlField *field;
    m_currentIndex = rowCount();
    for(int i = 0; i < SStoreItemModel::SaleOpColumns::ColRecordType + 1; i++)
    {
        field = new QSqlField("", QVariant::Int);
        switch(i)
        {
            case SStoreItemModel::SaleOpColumns::ColId: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColUID: field->setValue(""); break;
            case SStoreItemModel::SaleOpColumns::ColName: field->setValue(""); break;
            case SStoreItemModel::SaleOpColumns::ColCount: field->setValue(1); break;
            case SStoreItemModel::SaleOpColumns::ColAvail: field->setValue("999999"); break;
            case SStoreItemModel::SaleOpColumns::ColPrice: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColSumm: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColBox: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColSN: field->setValue(""); break;
            case SStoreItemModel::SaleOpColumns::ColWarranty: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColUser: field->setValue(userDbData->value("id").toInt()); break;
            case SStoreItemModel::SaleOpColumns::ColRealization: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColRetPercent: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColState: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColNotes: field->setValue(""); break;
            case SStoreItemModel::SaleOpColumns::ColItemId: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColInPrice: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColObjId: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColDealer: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColBuyer: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColCreated: field->setValue(QDateTime::currentDateTimeUtc()); break;
            case SStoreItemModel::SaleOpColumns::ColWorkId: field->setValue(0); break;
            case SStoreItemModel::SaleOpColumns::ColRecordType: field->setValue(RecordType::Work); break;
        }

        customWork->append(*field);
        delete field;
    }
    insertRecord(m_currentIndex, *customWork);
    delete customWork;
}

/* Добавление работы из прайс-листа
*/
bool SSaleTableModel::addWorkByUID(const int uid, const int priceOption)
{
//    QSqlQueryModel *work;
//    int row = -1;
    bool ret = 1;

//    if(m_modelState == SSaleTableModel::StoreNew || m_modelState == SSaleTableModel::StoreReserved || m_modelState == SSaleTableModel::WorkshopRW )
//    {
//        work = new QSqlQueryModel(this);
//        work->setQuery(QUERY_SEL_PART_FOR_SALE(uid, priceColModel->index(priceOption, 2).data().toString()), QSqlDatabase::database("connMain"));
//        work->record(0).setValue(SStoreItemModel::SaleOpColumns::ColObjId, m_objId);
//        ret = appendRecord(work->record(0));
//        delete work;
//    }

    return ret;
}

/*  Добавление товара по id
 *  В режиме простой продажи нельзя добавить один и тот же товар дважды (TODO: пересмотреть это решение)
 *  возвращает 0 в случае неудачи
*/
bool SSaleTableModel::addItemByUID(const int uid, const int priceOption, const int count)
{
    qDebug().nospace() << "[" << this << "] addItemByUID() | m_currentIndex = " << m_currentIndex;
    QSqlQueryModel *item = nullptr;
    int row = -1;
    bool ret = 1;
    int rowItemAlreadyInList;

    if(m_modelState == SSaleTableModel::StoreNew || m_modelState == SSaleTableModel::StoreReserved || m_modelState == SSaleTableModel::WorkshopRW || m_modelState == SSaleTableModel::WorkshopAdm )
    {
        try
        {
            item = new QSqlQueryModel(this);
            rowItemAlreadyInList = isItemAlreadyInList(uid);
            if(m_tableMode == TablesSet::StoreSale && rowItemAlreadyInList >= 0)
                throw 0;

            item->setQuery(QUERY_SEL_PART_FOR_SALE(uid, priceColModel->index(priceOption, 2).data().toString(), count), QSqlDatabase::database("connMain"));
            if(count > item->record(0).value("avail").toInt())
                throw 1;

            if(m_tableMode == TablesSet::StoreSale)
            {
                ret = appendRecord(item->record(0));
            }
            else
            {
                if(m_currentIndex == -1 && rowCount())  // если не выбрана строка и таблица не пуста
                    throw 2;

                ret = insertRecord(getItemInsertionRow(), item->record(0));
            }
        }
        catch (int exception)
        {
            QString msgCaption, msgText;
            if(exception == 0)
            {
                qDebug() << QString("товар UID %1 уже добавлен").arg(uid);
                msgCaption = tr("Повтор");
                msgText = tr("\"%1\" (UID %2) уже добавлен").arg(value(rowItemAlreadyInList, SStoreItemModel::SaleOpColumns::ColName).toString()).arg(uid);
            }
            else if(exception == 1)
            {
                qDebug() << QString("Запрошенное кол-во товара UID %1 не доступно").arg(item->record(0).value("UID").toString());
                msgCaption = tr("Товар отсутствует");
                msgText = tr("Запрошенное кол-во товара \"%1\" (UID %2) не доступно для продажи").arg(value(row, SStoreItemModel::SaleOpColumns::ColName).toString()).arg(value(row, SStoreItemModel::SaleOpColumns::ColUID).toString());
            }
            else if(exception == 2)
            {
                msgCaption = tr("Информация");
                msgText = tr("Не выбрана работа. Укажите работу в которой была использована деталь");
            }

            shortlivedNotification *newPopup = new shortlivedNotification(this, msgCaption, msgText, QColor(255,255,255), QColor(245,245,245));
            ret = 0;
        }
        if(item)
            delete item;
    }

    return ret;
}

bool SSaleTableModel::addItemFromBasket(const int id, const int qty, const int priceOpt)
{
    qDebug().nospace() << "[" << this << "] addItemFromBasket() | m_currentIndex = " << m_currentIndex;
    QSqlQueryModel *item = nullptr;
    bool ret = 1;
    int insertionRow;
    const QString count = qty?QString::number(qty):"`count`";
    if(m_modelState == SSaleTableModel::StoreNew || m_modelState == SSaleTableModel::StoreReserved || m_modelState == SSaleTableModel::WorkshopRW || m_modelState == SSaleTableModel::WorkshopAdm )
    {
        try
        {
            if(m_currentIndex == -1 && rowCount())  // если не выбрана строка и таблица не пуста
                throw 2;

            insertionRow = getItemInsertionRow();
            item = new QSqlQueryModel(this);
            item->setQuery(QUERY_SEL_PART_FROM_BASKET(id, priceColModel->index(priceOpt, 2).data().toString(), count), QSqlDatabase::database("connMain"));
            ret = insertRecord(insertionRow, item->record(0));
            if(qty) // добавление части товара из корзины сотрудника
            {
                /**/qDebug() << "[" << this << "] addItemFromBasket() | TODO: создать копию записи в табл. store_int_reserve с разницей кол-ва";
//                m_itemsPendingSplitList
                setData(index(insertionRow, SStoreItemModel::SaleOpColumns::ColCount), qty);   // обновить кол-во товара в старой записи
            }
        }
        catch (int exception)
        {
            QString msgCaption, msgText;
            if(exception == 2)
            {
                msgCaption = tr("Информация");
                msgText = tr("Не выбрана работа. Укажите работу в которой была использована деталь");
            }
            shortlivedNotification *newPopup = new shortlivedNotification(this, msgCaption, msgText, QColor(255,255,255), QColor(245,245,245));
            ret = 0;
        }

        if(item)
            delete item;
    }
    return ret;
}

void SSaleTableModel::removeRowHandler(const int row, const int db_id)
{
    if(m_tableMode == TablesSet::StoreSale)
    {
        if(m_modelState == SSaleTableModel::StoreNew)   // в режиме создания новой РН просто удаляем строки из модели
            removeRows(row, 1);
        else
            store_markRowRemove(row, db_id);
    }
    else
    {
        repair_markRowRemove(row, db_id);
        repair_autoSaveTables();
    }

    emit amountChanged(amountTotal(), m_amountItems, m_amountWorks);
    endResetModel();    // генерация сигнала modelReset() нужна для корректной раскраски строки, помеченной на удаление
}

void SSaleTableModel::buttonHandler(const int buttonNum, const int row)
{
    qDebug().nospace() << "[" << this << "] buttonHandler() | " << QString("buttonId = %1, row = %2").arg(buttonNum).arg(row);
    // действия при прямой продаже и в карте ремонта разные; формируем уникальный идентификатор
    int action = m_tableMode << 8 | index(row, SStoreItemModel::SaleOpColumns::ColRecordType).data().toInt() << 4 | buttonNum;
    switch(action)
    {
        case (StoreSale << 8    | RecordType::Item << 4 | 1):
        case (WorkshopSale << 8 | RecordType::Work << 4 | 0):
        case (WorkshopSale << 8 | RecordType::Item << 4 | 1): removeRowHandler(row, index(row, SStoreItemModel::SaleOpColumns::ColId).data().toInt()); break; // кнопка "удалить"
        case (WorkshopSale << 8 | RecordType::Work << 4 | 2): emit addItem(); break; // кнопка "добавить деталь"
        case (StoreSale << 8    | 2): ; break; // нет действия
    }
}

/* Удаление или пометка на удаление строки в режиме продажи: частичный возврат или частичная отмена
 * ранее зарезервированного товара.
 * При первом нажатии кнопки Удалить, в массив добавятся данные, а при повторном - удалятся (на случай,
 * если пользователь промахнулся). При подтверждении возврата или проведении РН резерва, записи с
 * соответствующими id будут обновлены, будет установлен флаг is_cancellation (state).
 * row  - номер строки
 * db_id - id записи в таблице store_sales
 */
void SSaleTableModel::store_markRowRemove(const int row, const int db_id)
{
    int newState = SStoreSaleItemModel::Cancelled;
    int newValue = 0;

    if(m_itemsPendingRemoveList->contains(row))
    {
        newState = SStoreSaleItemModel::Active;
        newValue = value(row, SStoreItemModel::SaleOpColumns::ColCount, DataRoles::OldValue).toInt();
        m_itemsPendingRemoveList->remove(row);
    }
    else
    {
        m_itemsPendingRemoveList->insert(row, db_id);
        setData(index(row, SStoreItemModel::SaleOpColumns::ColCount), value(row, SStoreItemModel::SaleOpColumns::ColCount), DataRoles::OldValue); // при пометке на удаление, в UserRole сохраняем текущее кол-во; это на случай, если пользователь промахнулся строкой и тут же нажал кнопку еще раз
    }
    setData(index(row, SStoreItemModel::SaleOpColumns::ColState), newState);
    setData(index(row, SStoreItemModel::SaleOpColumns::ColCount), newValue);   // кол-во устанавливаем 0
}

/* Пометка строки на удаление в режиме ремонта
 * row  - номер строки
 * db_id - id записей в таблицах store_int_reserve и works
*/
int SSaleTableModel::repair_markRowRemove(const int row, const int db_id)
{
    int newState = SRepairSaleItemModel::EngineerBasket;
    int recordType = index(row, SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool();
    QMap<int, int> *pendingRemoveList;
    qDebug().nospace() << "[" << this << "] repair_markRowRemove() | " << QString("row = %1, recordType = %2, m_currentIndex = %3").arg(row).arg(recordType).arg(m_currentIndex);
    if(recordType == RecordType::Work)   // сначала обрабатываем записи о товарах привязанных к удаляемой работе
    {
        pendingRemoveList = m_worksPendingRemoveList;
        int i = row + 1;
        while(i < rowCount() && index(i, SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool() == RecordType::Item)
        {
            i = repair_markRowRemove(i, index(i, SStoreItemModel::SaleOpColumns::ColId).data().toInt());
        }
    }
    else
        pendingRemoveList = m_itemsPendingRemoveList;

    if(m_editStrategy == OnManualSubmit && !index(row, SStoreItemModel::SaleOpColumns::ColObjId).data().toInt())
    {
        removeRows(row, 1);
        m_currentIndex = row - 1;
        return row;
    }
    else if(pendingRemoveList->contains(row))
    {   // в режиме ручного сохранения списка работ и деталей можно отменить пометку на удаление
        pendingRemoveList->remove(row);
        newState = SRepairSaleItemModel::RepairLinked;
    }
    else
    {
        pendingRemoveList->insert(row, db_id);
    }
    QStandardItemModel::setData(index(row, SStoreItemModel::SaleOpColumns::ColState), newState);
    return row + 1;
}

QMap<int, int>* SSaleTableModel::getPendingRemoveList()
{
    return m_itemsPendingRemoveList;
}

int SSaleTableModel::pendingRemoveItemsCount()
{
    if(m_itemsPendingRemoveList->isEmpty())
        return rowCount();

    return m_itemsPendingRemoveList->size();
}

/* Обновление цены
 * в данном методе для всех товаров, уже добавленных в таблицу, будут запрошен соответствующая колонка цен
 */
void SSaleTableModel::setPriceColumn(const int idx)
{
    m_priceIndex = idx;
    if(rowCount() == 0)
        return;

    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));

    query->prepare(QUERY_SEL_STORE_ITEMS_ITEM_PRICE(priceColModel->index(idx, 2).data().toString()));
    for(int i = 0; i < rowCount(); i++)
    {
        query->bindValue(":id", index(i,  SStoreItemModel::SaleOpColumns::ColItemId).data().toInt());
        query->exec();
        query->first();
        setData(index(i,  SStoreItemModel::SaleOpColumns::ColPrice), query->record().value(0).toDouble());
    }
    delete query;
}

/* Загрузка таблицы товаров ранее сохранённого документа (продажа)
 * Возвращает 1 в случае ошибки
*/
bool SSaleTableModel::store_loadTable(const int doc_id)
{
    setTableMode(TablesSet::StoreSale);
    setDocumentId(doc_id);
    if(m_modelState == State::StoreSold)
        m_queryData->setQuery(QUERY_SEL_ITEMS_IN_DOC(m_objId), QSqlDatabase::database("connMain"));
    else if( m_modelState == State::StoreReserved || m_modelState == State::StoreCancelled )
        m_queryData->setQuery(QUERY_SEL_ITEMS_IN_DOC_RSRV(m_objId), QSqlDatabase::database("connMain"));
    else
        return 1;

    return 0;
}

/* Загрузка таблицы товаров ремонта
 * Возвращает 1 в случае ошибки
*/
bool SSaleTableModel::repair_loadTable(const int repair_id)
{
    setRepairId(repair_id);
    m_queryData->setQuery(QUERY_SEL_REPAIR_WORKS_AND_PARTS(m_objId), QSqlDatabase::database("connMain"));

    return 0;
}

void SSaleTableModel::setClient(int id)
{
    m_client = id;
}

void SSaleTableModel::unsetClient()
{
//    values.insert("client", "NULL");
    m_client = 0;
}

void SSaleTableModel::setDocumentId(int id)
{
    m_objId = id;
}

void SSaleTableModel::setRepairId(int id)
{
    m_objId = id;
}

/*  Продажа (в т. ч. ранее зарезервированных) и резерв товаров.
 *  Вызов метода происходит в результате нажатия кнопки пользователем (т. е. сохранение не происходит автоматически)
 *  Допускается, что покупатель откажется от части зарезервированных товаров (помеченные пользователем строки)
 *  Возвращает 0 в случае ошибки
 */
bool SSaleTableModel::store_saveTables(StoreOpType type)
{
    bool ret = 1;

    if(!m_itemsPendingRemoveList->isEmpty())
        ret = store_backOutItems(StoreOpType::FreeReserved);

    m_itemsAffected = 0;
    for(int i = 0; i < rowCount() && ret; i++)
    {
        if(index(i, SStoreItemModel::SaleOpColumns::ColState).data().toBool())  // частично снятые с резерва пропускаем
            continue;

        m_itemsAffected++;
        setData(index(i, SStoreItemModel::SaleOpColumns::ColObjId), m_objId);
        setData(index(i, SStoreItemModel::SaleOpColumns::ColBuyer), m_client);
        SStoreSaleItemModel *itm = store_item(i);
        if(type == StoreOpType::Sale)
            ret = itm->sale();
        else
            ret = itm->reserve();

        delete itm;
    }

    if(!ret)
        throw 1;

    clearChangedFlagForAllField();
    return ret;
}

/*  Сохранение таблицы работ и деталей в ремонте.
 *  Вызов метода может происходить, в зависимости от настроек пользователя, при:
 *   - завершении редактирования таблицы (т. е. автоматически после любого изменения; по умолчанию)
 *   - TODO: вручную.
 *  Возвращает 0 в случае ошибки
 */
bool SSaleTableModel::repair_saveTables()
{
    bool ret = 1;
    bool rowModified = 0;
    int lastHandledWorkId = 0, lastInsertId = 0;
    int editStrategyBackup = m_editStrategy;

    qDebug().nospace() << "[" << this << "] repair_saveTables()";
    ret = repair_removeRows();

    // Перед вызовом методов записи данных в БД нужно дополнительно задать значения некоторых полей
    // и при этом избежать зацикливания, если включен режим сохранения после каждого изменения
    m_editStrategy = EditStrategy::Nop;

    for(int i = 0; i < rowCount() && ret; i++)
    {
        rowModified = index(i, SStoreItemModel::SaleOpColumns::ColId).data(Changed).toBool();

        if(!index(i, SStoreItemModel::SaleOpColumns::ColObjId).data().toInt())
            setData(index(i, SStoreItemModel::SaleOpColumns::ColObjId), m_objId);

        if(index(i, SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool() == RecordType::Work)
        {

            SWorkModel *work = repair_work(i);
            if(rowModified)
                ret = work->commit();
            lastHandledWorkId = work->id();
            lastInsertId = lastHandledWorkId;
            delete work;
        }
        else if(rowModified)   // обработка записи товара (только изменённые строки)
        {
            setData(index(i, SStoreItemModel::SaleOpColumns::ColWorkId), lastHandledWorkId);
            setData(index(i, SStoreItemModel::SaleOpColumns::ColState), SRepairSaleItemModel::RepairLinked);
            SRepairSaleItemModel *itm = repair_item(i);

            ret &= itm->commit();
            lastInsertId = itm->id();
            delete itm;
        }
        if(!index(i, SStoreItemModel::SaleOpColumns::ColId).data().toInt())
            QStandardItemModel::setData(index(i, SStoreItemModel::SaleOpColumns::ColId), lastInsertId);

    }

    m_editStrategy = editStrategyBackup;

    if(!ret)
        throw 1;

    clearChangedFlagForAllField();
    emit tableSaved();

    return ret;
}

/* Данный метод предназначен для сохранения данных таблицы автономно
 * (отличается от repair_saveTables отправкой запросов начала и завершения транзакции).
*/
bool SSaleTableModel::repair_saveTablesStandalone()
{
    bool nErr = 1;

    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));

#ifdef QT_DEBUG
    SQueryLog *queryLog = new SQueryLog();
    queryLog->start(metaObject()->className());
#endif

    try
    {
        QUERY_EXEC(query,nErr)(QUERY_BEGIN);

        repair_saveTables();

#ifdef QT_DEBUG
//            throw 0; // это для отладки (чтобы сессия всегда завершалась ROLLBACK'OM)
#endif

        QUERY_COMMIT_ROLLBACK(query,nErr);
    }
    catch(int type)
    {
        nErr = 0;

        if(type == 0)
        {
            QString err = "DEBUG ROLLBACK";
            QUERY_ROLLBACK_MSG(query, err);
        }
        else
            QUERY_COMMIT_ROLLBACK(query, nErr);
    }
#ifdef QT_DEBUG
    queryLog->stop();
    delete queryLog;
#endif

    if(nErr)
    {
        shortlivedNotification *newPopup = new shortlivedNotification(this, tr("Успешно"), tr("Список работ и деталей сохранён"), QColor(214,239,220), QColor(229,245,234));
    }
    delete query;

    return nErr;
}

/*  Метод сохранения таблицы для случаев выдачи ремонта или возврата ранее выданного ремонта
 */
bool SSaleTableModel::repair_saveTables(RepairOpType operation)
{
    qDebug().nospace() << "[" << this << "] repair_saveTables(RepairOpType)";
    bool ret = 1;

    ret = repair_saveTables();   // если по каким-либо причинам таблица не сохранена
    for(int i = 0; i < rowCount() && ret; i++)
    {
        qDebug().nospace() << "[" << this << "] repair_saveTables(RepairOpType) | " << QString("i = %1").arg(i);
        if(index(i, SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool() == RecordType::Work)
            continue;

        SRepairSaleItemModel *itm = repair_item(i);
        switch(operation)
        {
            case RepairOpType::Sale: ret = itm->sale(); break;
            case RepairOpType::Unsale: ret = itm->unsale(); break;
            default:;
        }
        delete itm;
    }

    if(!ret)
        throw 1;

    clearChangedFlagForAllField();
    return ret;
}

/*  В данном методе производится вызов метода repair_saveTablesStandalone()
 *  или эмиссия сигнала tableDataChanged(), в зависимости от режима сохранения
*/
bool SSaleTableModel::repair_autoSaveTables()
{
    qDebug().nospace() << "[" << this << "] repair_autoSaveTables()";
    bool nErr = 1;

    if( m_editStrategy == OnFieldChange || m_editStrategy == OnRowChange )
    {
        nErr = repair_saveTablesStandalone();
    }
    else if(m_editStrategy == OnManualSubmit)
    {
        m_unsaved = 1;
        emit tableDataChanged();
    }

    return nErr;
}

bool SSaleTableModel::reserveItems()
{
    if(m_tableMode == TablesSet::WorkshopSale)
        return 1;

    return store_saveTables(StoreOpType::Reserve);
}

/* Возврат товара
 * Допускается полный возврат товара (если пользователь не пометил отдельные cтроки)
 * или частичный. Также допускается многократный частичный возврат.
 */
bool SSaleTableModel::unsaleItems()
{
    if(m_tableMode == TablesSet::WorkshopSale)
        return true;

    if(m_itemsPendingRemoveList->isEmpty())
    {
        store_markAllItemsToRemove(StoreOpType::Unsale);
    }
    return store_backOutItems(StoreOpType::Unsale);
}

/*
 * reason — причина возврата, указанная пользователем; будет записана в журнал
*/
bool SSaleTableModel::unsaleItems(const QString &reason)
{
    setExtraUnsaleReason(reason);
    unsaleItems();
}

void SSaleTableModel::setExtraUnsaleReason(const QString &reason)
{
    m_extraUnsaleReason = reason;
}

/* Полное снятие резерва
 * Очередь на удаление очищается и заполняется снова всеми строками
 */
bool SSaleTableModel::freeItems()
{
    if(m_tableMode == TablesSet::WorkshopSale)
        return true;

    m_itemsPendingRemoveList->clear();
    store_markAllItemsToRemove(StoreOpType::FreeReserved);
    return store_backOutItems(StoreOpType::FreeReserved);
}

/*  В данном методе производится непосредственно возврат/снятие резерва
 */
bool SSaleTableModel::store_backOutItems(StoreOpType type)
{
    bool ret = 1;
    m_itemsAffected = 0;

    QMap<int, int>::const_iterator i;
    for (i = m_itemsPendingRemoveList->constBegin(); i != m_itemsPendingRemoveList->constEnd() && ret; ++i)
    {
        m_itemsAffected++;
        SStoreSaleItemModel *itm = store_item(i.key());
        if(type == StoreOpType::Unsale)
        {
            itm->setExtraUnsaleReason(m_extraUnsaleReason);
            ret = itm->unsale();
        }
        else
            ret = itm->free();

        delete itm;
    }

    if(ret)
        m_itemsPendingRemoveList->clear();
    else
        m_itemsAffected = 0;

    if(!ret)
        throw 1;

    return ret;
}

bool SSaleTableModel::isRowMarkedRemove(const int row) const
{
    int state = value(row, SStoreItemModel::SaleOpColumns::ColState).toInt();
    if((m_tableMode == SSaleTableModel::WorkshopSale && state == SRepairSaleItemModel::EngineerBasket) || \
       (m_tableMode == SSaleTableModel::StoreSale && state == SStoreSaleItemModel::Cancelled))
        return 1;

    return 0;
}

bool SSaleTableModel::repair_removeRows()
{
    qDebug().nospace() << "[" << this << "] repair_removeRows()";
    bool ret = 1;

    if(ret && !m_itemsPendingRemoveList->isEmpty())
    {
        ret = repair_removeItems();
        m_currentIndex = m_itemsPendingRemoveList->firstKey() - 1;
    }
    if(ret && !m_worksPendingRemoveList->isEmpty())
    {
        ret = repair_removeWorks();
        m_currentIndex = qMin((m_worksPendingRemoveList->firstKey() - 1), m_currentIndex);
    }


    for(int i = rowCount() - 1; i >= 0 && ret; i--)
    {
        if(isRowMarkedRemove(i))
            removeRows(i, 1);
    }

    if(ret)
    {
        m_itemsPendingRemoveList->clear();
        m_worksPendingRemoveList->clear();
    }

    endResetModel();
    return ret;
}

/*  Обработка очереди на удаление работ/товаров
 *  В случае сбоя вызывается исключение
*/
bool SSaleTableModel::repair_removeItems()
{
    qDebug().nospace() << "[" << this << "] repair_removeItems()";
    bool nErr = 1;

    QMap<int, int>::const_iterator i;
    for (i = m_itemsPendingRemoveList->constBegin(); i != m_itemsPendingRemoveList->constEnd() && nErr; ++i)
    {
        SRepairSaleItemModel *itm = repair_item(i.key());
        nErr = itm->unlinkRepair();
        delete itm;

#ifdef QT_DEBUG
//    nErr = 0;    // для отладки режима удаления
#endif

        if(!nErr)
            throw 1;
    }

    return nErr;
}

bool SSaleTableModel::repair_removeWorks()
{
    qDebug().nospace() << "[" << this << "] repair_removeWorks()";
    bool nErr = 1;

    QMap<int, int>::const_iterator i;
    for (i = m_worksPendingRemoveList->constBegin(); i != m_worksPendingRemoveList->constEnd() && nErr; ++i)
    {
        SWorkModel *itm = repair_work(i.key());
        nErr = itm->remove();
        delete itm;

#ifdef QT_DEBUG
//    nErr = 0;    // для отладки режима удаления
#endif

        if(!nErr)
            throw 1;
    }

    return nErr;
}

void SSaleTableModel::sale()
{
    if(m_tableMode == TablesSet::WorkshopSale)
        return;

    store_saveTables(StoreOpType::Sale);
}

int SSaleTableModel::itemsAffected()
{
    return m_itemsAffected;
}

/*  Помещает все строки таблицы в список на удаление
 */
void SSaleTableModel::store_markAllItemsToRemove(StoreOpType type)
{
    for(int i = 0; i < rowCount(); i++)
    {
        if(type == StoreOpType::Unsale)
            if(index(i, SStoreItemModel::SaleOpColumns::ColState).data().toBool())  // возвращённые ранее пропускаем; касается только простых продаж
                continue;

        removeRowHandler(i, index(i, SStoreItemModel::SaleOpColumns::ColId).data().toInt());
    }
}

SStoreSaleItemModel *SSaleTableModel::store_item(const int rownum)
{
    SStoreSaleItemModel *item = new SStoreSaleItemModel(row(rownum), parent());
    return item;
}

SRepairSaleItemModel *SSaleTableModel::repair_item(const int rownum)
{
    SRepairSaleItemModel *item = new SRepairSaleItemModel(row(rownum));
    return item;
}

SWorkModel *SSaleTableModel::repair_work(const int rownum)
{
    SWorkModel *item = new SWorkModel(row(rownum));
    return item;
}

int SSaleTableModel::tableMode()
{
    return m_tableMode;
}

void SSaleTableModel::setTableMode(const TablesSet mode)
{
    m_tableMode = mode;
    if(mode == TablesSet::StoreSale)
    {
        m_hiddenColumns = 1 << SStoreItemModel::SaleOpColumns::ColUser | 1 << SStoreItemModel::SaleOpColumns::ColRealization | 1 << SStoreItemModel::SaleOpColumns::ColRetPercent | 1 << SStoreItemModel::SaleOpColumns::ColState | 1 << SStoreItemModel::SaleOpColumns::ColNotes | 1 << SStoreItemModel::SaleOpColumns::ColItemId | 1 << SStoreItemModel::SaleOpColumns::ColInPrice | 1 << SStoreItemModel::SaleOpColumns::ColObjId | 1 << SStoreItemModel::SaleOpColumns::ColDealer | 1 << SStoreItemModel::SaleOpColumns::ColBuyer | 1 << SStoreItemModel::SaleOpColumns::ColRecordType | 1 << SStoreItemModel::SaleOpColumns::ColCreated | 1 << SStoreItemModel::SaleOpColumns::ColWorkId;
        m_editStrategy = EditStrategy::OnManualSubmit; // режим редактирования при продаже всегда OnManualSubmit
    }
    else
    {
        m_hiddenColumns = 1 << SStoreItemModel::SaleOpColumns::ColAvail | 1 << SStoreItemModel::SaleOpColumns::ColRealization | 1 << SStoreItemModel::SaleOpColumns::ColRetPercent | 1 << SStoreItemModel::SaleOpColumns::ColState | 1 << SStoreItemModel::SaleOpColumns::ColNotes | 1 << SStoreItemModel::SaleOpColumns::ColItemId | 1 << SStoreItemModel::SaleOpColumns::ColInPrice | 1 << SStoreItemModel::SaleOpColumns::ColObjId | 1 << SStoreItemModel::SaleOpColumns::ColDealer | 1 << SStoreItemModel::SaleOpColumns::ColBuyer | 1 << SStoreItemModel::SaleOpColumns::ColRecordType | 1 << SStoreItemModel::SaleOpColumns::ColCreated | 1 << SStoreItemModel::SaleOpColumns::ColWorkId;
    }
}

bool SSaleTableModel::isColumnHidden(const int column)
{
    return (m_hiddenColumns >> column)&0x01;
}

int SSaleTableModel::editStrategy()
{
    return m_editStrategy;
}

void SSaleTableModel::setEditStrategy(const int strategy)
{
    m_editStrategy = strategy;
}

bool SSaleTableModel::isWarranty()
{
    return m_isWarranty;
}

void SSaleTableModel::setIsWarranty(const bool isWarranty)
{
    m_isWarranty = isWarranty;
}

bool SSaleTableModel::isUnsaved()
{
    qDebug().nospace() << "[" << this << "] isUnsaved() | m_unsaved = " << m_unsaved;
    return m_unsaved;
}

QList<QStandardItem *> SSaleTableModel::row(int row) const
{
    QList<QStandardItem *> rowItems;
    for(int column = 0; column < columnCount(); column++)
    {
        rowItems << QStandardItemModel::item(row, column);
    }

    return rowItems;
}

/* Возвращает номер строки для добавления товара.
*/
int SSaleTableModel::getItemInsertionRow()
{
    int rows = rowCount();

    // если таблица пуста, то добавляем произвольную работу
    if(!activeRowCount())
    {
        int editStrategyBackup = m_editStrategy;
        m_editStrategy = EditStrategy::Nop;
        addCustomWork();
        m_editStrategy = editStrategyBackup;
    }

    // если в таблице только одна запись или следующая строка соотв. тоже работе, то вставить деталь нужно перед ней
    if(m_currentIndex + 1 == rows || !recordType(m_currentIndex + 1))
    {
        return m_currentIndex + 1;
    }

    // с текущей выбранной строки производим поиск строки с работой
    for(int i = m_currentIndex+1; i < rows; i++)
    {
        if(!recordType(i))
        {
            return i;
        }
    }

    // если такая строка не найдена, то добавляем в конец таблицы
    return rows;
}

int SSaleTableModel::getParentWorkRow(const int itemRow)
{
    int i;
    for(i = itemRow; i >= 0; i--)
    {
        if(recordType(i) == RecordType::Work)
            break;
    }
    return i;
}

bool SSaleTableModel::recordType(const int row)
{

    return index(row, SStoreItemModel::SaleOpColumns::ColRecordType).data().toBool();
}

void SSaleTableModel::clearChangedFlagForAllField()
{
    for(int i = 0; i < rowCount(); i++)
    {
        if(index(i, 0).data(DataRoles::Changed).toBool())
        {
            for(int j = 1; j < columnCount(); j++) // в нулевом столбце — id записи в таблице, он не изменяется средствами программы
            {
                if(index(i, j).data(DataRoles::Changed).toBool())
                {
                    QStandardItemModel::setData(index(i, j), 0, DataRoles::Changed);   // снятие флага о наличии изменений в поле
                    QStandardItemModel::setData(index(i, j), QVariant(), DataRoles::OldValue);   // очистка старого значения
                }
            }
            QStandardItemModel::setData(index(i, SStoreItemModel::SaleOpColumns::ColId), 0, DataRoles::Changed);   // снятие флага о наличии изменений в строке
        }
    }
    m_unsaved = 0;
}

/*  Возвращает кол-во строк в таблице, не помеченных на удаление
 *  (данный метод нужен для работы в режиме OnManualSubmit)
*/
int SSaleTableModel::activeRowCount() const
{
    int rows = 0;
    for(int i = 0; i < rowCount(); i++)
    {
        if(isRowMarkedRemove(i))
            continue;
        rows++;
    }
    return rows;
}

void SSaleTableModel::indexSelected(const QModelIndex &index)
{
    m_currentIndex = index.row();
}

/* т. к. некоторые данные обрабатываются по логическому номеру столбца, важно, чтобы они не изменились
 * В этом методе будет производиться сравнение со значениями из списка по умолчанию
 * TODO: вообще, нужны более универсальные способы получения данных конкретного столбца, но это потом...
 */
void SSaleTableModel::setHorizontalHeaderLabels()
{
    QStringList labels = {"",tr("UID"),tr("Наименование"),tr("Кол-во"),tr("Доступно"),tr("Цена"),tr("Сумма"),tr("Место"),tr("Серийный номер"),tr("Гарантия"),tr("Сотрудник")};
    QStandardItemModel::setHorizontalHeaderLabels(m_fieldsDep);
    for(int i = 0; i < labels.count(); i++)
    {
        Q_ASSERT_X(horizontalHeaderItem(i)->text() == m_fieldsDep.at(i), "SSaleTableModel::setHorizontalHeaderLabels()", "fields dependencies");
    }
    QStandardItemModel::setHorizontalHeaderLabels(labels);
}

/* Сумма всех товаров
 */
double SSaleTableModel::amountTotal()
{
    double summ;
    m_amountTotal = 0;
    m_amountItems = 0;
    m_amountWorks = 0;
    for(int i = 0; i < rowCount(); i++)
    {
        if(isRowMarkedRemove(i))
            continue;

        summ = value(i, SStoreItemModel::SaleOpColumns::ColSumm).toDouble();
        if(value(i, SStoreItemModel::SaleOpColumns::ColRecordType).toBool())
            m_amountItems += summ;
        else
            m_amountWorks += summ;

        m_amountTotal += summ;
    }
    return m_amountTotal;
}

QString SSaleTableModel::amountTotalLocale()
{
    return sysLocale.toString(amountTotal(), 'f', 2);
}

double SSaleTableModel::amountItems()
{
    return m_amountItems;
}

QString SSaleTableModel::amountItemsLocale()
{
    return sysLocale.toString(m_amountItems, 'f', 2);
}

QString SSaleTableModel::amountWorksLocale()
{
    return sysLocale.toString(m_amountWorks, 'f', 2);
}

QVariant SSaleTableModel::value(const int row, const int column, const int role) const
{
    return QStandardItemModel::data(index(row, column), role);
}

void SSaleTableModel::setModelState(int state)
{
    m_modelState = state;
    modelStateChanged(m_modelState);
    endResetModel();    // для перерисовки кнопок в таблице
}

int SSaleTableModel::modelState()
{
    return m_modelState;
}

/* В этом слоте происходит копирование данных из QSqlQueryModel в QStandardItemModel
 * Для сохранённых данных будет достаточно вызвать метод this->setQuery()
 *
 */
void SSaleTableModel::sqlDataChanged()
{
    clear();
    setHorizontalHeaderLabels();
        setRowCount(m_queryData->rowCount());
    setColumnCount(m_queryData->columnCount());
    m_itemsPendingRemoveList->clear();
    this->blockSignals(true);
    for(int i = 0; i < m_queryData->rowCount(); i++)
    {
        for(int j = 0; j < m_queryData->columnCount(); j++)
        {
            QStandardItemModel::setData(index(i, j), m_queryData->index(i, j).data());
        }
        // Чтобы лишний раз не лезть в другие индексы объекта, в UserRole записываем значение is_cancellation (state);
        // оно будет использовано для отрисовки кнопки: товары, возврат которых был оформлен при предыдущем
        // администрировании документа, придётся продать в новом документе.
        QStandardItemModel::setData(index(i, 0), isRowMarkedRemove(i), DataRoles::State);
        QStandardItemModel::setData(index(i, 0), m_queryData->index(i, SStoreItemModel::SaleOpColumns::ColRecordType).data(), DataRoles::RecordType);
    }
    this->blockSignals(false);
    m_queryData->blockSignals(true);
    m_queryData->clear();
    m_queryData->blockSignals(false);
    emit amountChanged(amountTotal(), m_amountItems, m_amountWorks);

    endResetModel();
}

/*  Обновление числа в столбце "Сумма" при изменении чисел в стобцах "Кол-во" или "Цена"
*/
#if QT_VERSION >= 0x060000
void SSaleTableModel::dataChangedHook(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
#else
void SSaleTableModel::dataChangedHook(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
#endif
{
    int row = topLeft.row();
//    qDebug().nospace() << "[" << this << "] dataChangedHook() | TopLeft[R,C] = [" << topLeft.row() << "," << topLeft.column() << "] | BottomRight[R,C] = [" << bottomRight.row() << "," << bottomRight.column() << "] | changed flag = " << index(row, SStoreItemModel::SaleOpColumns::ColId).data(DataRoles::Changed ).toBool();
    int column = topLeft.column();
    if( (column == SStoreItemModel::SaleOpColumns::ColCount || column == SStoreItemModel::SaleOpColumns::ColPrice) /*&& m_amountChangedSignalFilter*/ )   // был глюк, что без фильтра при добавлении первого товара в таблицу не обновляется общая сумма документа
    {
        QStandardItemModel::setData(index(row, SStoreItemModel::SaleOpColumns::ColSumm), value(row, SStoreItemModel::SaleOpColumns::ColCount).toInt() * value(row, SStoreItemModel::SaleOpColumns::ColPrice).toDouble() );
        emit amountChanged(amountTotal(), m_amountItems, m_amountWorks);
    }
//    qDebug().nospace() << "[" << this << "] dataChangedHook() | changed flag = " << index(row, SStoreItemModel::SaleOpColumns::ColId).data(DataRoles::Changed ).toBool();
}

bool SSaleTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool ret = 0;
    if (role == Qt::EditRole)
    {
        qDebug().nospace() << "[" << this << "] setData() | " << QString("index[%1, %2]").arg(index.row()).arg(index.column());
        if (!checkIndex(index))
            return ret;

        // если ячейки "Кол-во" и "Доступно" ранее были подсвечены ошибочными, то после редактирования сбрасываем фоновый цвет
        if( index.column() == SStoreItemModel::SaleOpColumns::ColCount)
        {
            if(index.data(Qt::BackgroundRole) == QColor(255,209,209) )
            {
                QStandardItemModel::setData(index, QVariant(), Qt::BackgroundRole);
                QStandardItemModel::setData(this->index(index.row(), SStoreItemModel::SaleOpColumns::ColAvail), QVariant(), Qt::BackgroundRole);
            }
        }

        // то же для ячекйки "Цена"
        if( index.column() == SStoreItemModel::SaleOpColumns::ColPrice)
        {
            if(index.data(Qt::BackgroundRole) == QColor(255,209,209) )
            {
                QStandardItemModel::setData(index, QVariant(), Qt::BackgroundRole);
            }
        }

        qDebug().nospace() << "[" << this << "] setData() | index.data = " << index.data(Qt::DisplayRole) << "; value = " << value;
        if(index.data(Qt::DisplayRole) != value)
        {
            QStandardItemModel::setData(index, 1, Changed); // пометка поля изменённым
            if( !index.data(DataRoles::OldValue).isValid() && \
                (   this->index(index.row(), SStoreItemModel::SaleOpColumns::ColObjId).data().toInt() || \
                    (   this->index(index.row(), SStoreItemModel::SaleOpColumns::ColId).data().toInt() &&\
                        !this->index(index.row(), SStoreItemModel::SaleOpColumns::ColObjId).data().toInt()
                    ))) {
                QStandardItemModel::setData(index, QStandardItemModel::data(index), DataRoles::OldValue); // сохраняем старое значение (для записи в журнал)
            }
            QStandardItemModel::setData(this->index(index.row(), SStoreItemModel::SaleOpColumns::ColId), 1, Changed); // пометка строки изменённой
        }

        ret = QStandardItemModel::setData(index, value);
        if(ret)
            repair_autoSaveTables();

        return ret;
    }

    ret = QStandardItemModel::setData(index, value, role);
    return ret;
}

void SSaleTableModel::setQuery(const QString &query, const QSqlDatabase &db)
{
    m_queryData->setQuery(query, db);
}

#ifdef QT_DEBUG
void SSaleTableModel::dbgAddRandomItem()
{
    int i;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));
    if(m_tableMode == SSaleTableModel::StoreSale)
    {   // при простой продаже добавление от 1 до 3 случайных товаров
        i = QRandomGenerator::global()->bounded(3); // кол-во товаров от 1 до 3
        if(i < 3)
            i += 3;
    }
    else    // в ремонт добавление одного случайного товара
        i = 3;

    for(int j = 0; j < i; j++)
    {
        query->exec(QString("SELECT `id` FROM (SELECT ROUND(@i * RAND(), 0) AS 'rand') AS `rand` LEFT JOIN (SELECT @i := @i + 1 AS 'num', t1.`id` FROM store_items AS t1 CROSS JOIN (SELECT @i := 0) AS dummy WHERE t1.`count` - t1.`reserved` > 0 AND t1.`is_realization` = 1) AS t1 ON t1.`num` = `rand`.`rand`;"));
        if(j<2)
            continue;   // после обновления сервера на mysql 5.6.51 (win) пришлось чуть-чуть изменить запрос для случайного товара; также в только что открытой сессии результаты первых двух запросов будут состоять из NULL, поэтому пропускаем их

        query->first();
        if(query->isValid())
            addItemByUID(query->record().value(0).toInt(), m_priceIndex);
//            addItemByUID(28147, m_priceIndex);
    }
    delete query;
}

void SSaleTableModel::dbgAddRandomItemBasket()
{
    int i;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));

    for(int j = 0; j < 3; j++)
    {
        query->exec(QString("SELECT `id` FROM (SELECT ROUND(@i * RAND(), 0) AS 'rand') AS `rand` LEFT JOIN (SELECT @i := @i + 1 AS 'num', t1.`id` FROM store_int_reserve AS t1 CROSS JOIN (SELECT @i := 0) AS dummy WHERE t1.`state` = 1) AS t1 ON t1.`num` = `rand`.`rand`;"));
        if(j<2)
            continue;   // после обновления сервера на mysql 5.6.51 (win) пришлось чуть-чуть изменить запрос для случайного товара; также в только что открытой сессии результаты первых двух запросов будут состоять из NULL, поэтому пропускаем их

        query->first();
        if(query->isValid())
            addItemFromBasket(query->record().value(0).toInt(), 0, m_priceIndex);
//            addItemFromBasket(19728, 0, m_priceIndex);
    }
    delete query;
}
#endif
