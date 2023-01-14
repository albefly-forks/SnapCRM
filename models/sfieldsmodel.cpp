#include "sfieldsmodel.h"

SFieldsModel::SFieldsModel(Type type, QObject *parent) :
    QObject(parent),
    m_type(type)
{
    query = new QSqlQuery(QSqlDatabase::database("connMain"));
}

SFieldsModel::~SFieldsModel()
{
    delete query;
    clear();
}

void SFieldsModel::clear()
{
    SFieldValueModel *f;

    while(!m_fieldsList.isEmpty())
    {
        f = m_fieldsList.last();
        m_fieldsList.removeLast();
        delete f;
    }
}

QList<SFieldValueModel*> SFieldsModel::list()
{
    return m_fieldsList;
}

/*  Инициализация массива виджетов для нового товара или ремонта
 *  isRepair == 1 — инициализация виджетов для класса <id> устройств,
 *  иначе для <id> категории товаров
 */
bool SFieldsModel::init(const int id)
{
    if(id == 0)
        return 0;

    if( !m_fieldsList.isEmpty() )
    {
        clear();
    }

    SFieldValueModel *field;

    query->exec(QUERY_SEL_ADDITIONAL_FIELDS_TYPES((m_type?1:0), id));
    while(query->next())
    {
        field = new SFieldValueModel();
        field->createWidget(query->record());
        add(field);
    }
    return 1;
}

bool SFieldsModel::load(int id)
{
    clear();
    if(m_type == Type::Repair)
        query->exec(QUERY_SEL_REPAIR_ADD_FIELDS(id));
    else
        query->exec(QUERY_SEL_ITEM_ADD_FIELDS(id));

    while(query->next())
    {
        itemHandler(query->record());
    }

    return 1;
}

void SFieldsModel::add(SFieldValueModel *item)
{
    m_fieldsList.append(item);
}

void SFieldsModel::remove(SFieldValueModel *item)
{
    m_removeList.append(item);
    int index = m_fieldsList.indexOf(item);
    m_fieldsList.removeAt(index);

}

bool SFieldsModel::isEmpty()
{
    return m_fieldsList.isEmpty();
}

void SFieldsModel::setObjectId(const int id)
{
    SFieldValueModel *f;
    foreach(f, m_fieldsList)
    {
        if(m_type == Type::Repair)
            f->setRepairId(id);
        else
            f->setItemId(id);
    }
}

bool SFieldsModel::commit()
{
    SFieldValueModel *item;
    foreach(item, m_fieldsList)
    {
        if(!item->commit())
        {
            m_nErr = 0;
            throw 1;
        }
    }

    while( !m_removeList.isEmpty() )
    {
        item = m_removeList.last();
        if(!item->delDBRecord())
        {
            m_nErr = 0;
            throw 1;
        }

        m_removeList.removeLast();
        item->deleteLater();
    }

    return 1;
}

bool SFieldsModel::validate()
{
    bool ret = 1;
    SFieldValueModel *item;
    foreach(item, m_fieldsList)
    {
        ret &= item->validate();
    }
    return ret;
}

void SFieldsModel::resetIds()
{
    SFieldValueModel *item;
    foreach(item, m_fieldsList)
    {
        if(!item->value().isEmpty())
            item->setId(0);
    }
}

void SFieldsModel::enableEdit()
{
    SFieldValueModel *item;
    foreach(item, m_fieldsList)
    {
        connect(item, SIGNAL(emptied(SFieldValueModel*)), this, SLOT(remove(SFieldValueModel*)));
    }
}

SFieldValueModel *SFieldsModel::itemHandler(const QSqlRecord &record)
{
    SFieldValueModel *item = new SFieldValueModel(this);
    item->load(record);
    add(item);
    return item;
}

#ifdef QT_DEBUG
void SFieldsModel::randomFill()
{
    for(int j=0; j< m_fieldsList.size(); j++)   // автозаполнение обязательных доп. полей
    {
        if ( m_fieldsList[j]->property("fieldRequired").toBool() )
            m_fieldsList[j]->randomFill();
    }
}
#endif
