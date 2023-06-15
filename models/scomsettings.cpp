#include "scomsettings.h"
#include "global.h"

SComSettings::SComSettings() :
    SPropertyCollection()
{
}

void SComSettings::initWidgets()
{
    for(int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); i++)
        metaObject()->property(i).read(this);   // при первом вызове метода READ происходит инициализация виджетов

    // Модели данных виджетов (ComboBox) должны быть заданы до загрузки данных, иначе будет падать.
///*DBG*/   static_cast<QComboBox*>(i_editorWidgets.value("currency"))->setModel(warrantyTermsModel);

    load();
}

int SComSettings::fieldToPropertyId(const QString &fieldName)
{
    QString propertyName = i_fieldNames.key(fieldName);
    if(propertyName.isEmpty())
    {
        appLog->appendRecord(QString("Unknown setting \"%1\"").arg(fieldName));
        return -1;
    }

    return metaObject()->indexOfProperty((&propertyName)->toLocal8Bit());
}

void SComSettings::load()
{
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));
    int i = 0;

    for(i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); i++)
        metaObject()->property(i).write(this, 0);   // при первом вызове методов WRITE происходит регистрация имён полей таблиц БД

    // "переворот" таблицы config
    query->exec("SET @smth := NULL");
    query->exec("SELECT @smth := CONCAT(IF(@smth IS NOT NULL, CONCAT(@smth, '\\\nUNION ALL\\\n'), ''), 'SELECT \\\'', `COLUMN_NAME`, '\\\', `', `COLUMN_NAME`, '` FROM `config`') FROM information_schema.`COLUMNS` WHERE `TABLE_SCHEMA` = 'latestbackup' AND `TABLE_NAME` = 'config';");

    query->last();
    query->exec(query->value(0).toString());
    while (query->next())
    {
        i = fieldToPropertyId(query->value(0).toString());
        if(i < 0)
            continue;

        metaObject()->property(i).write(this, query->value(1));
    }

    loadFromTableSettings();

    loadFromJson();

    delete query;
}

void SComSettings::loadFromTableSettings()
{
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));
    int i = 0;

    query->exec("SELECT `name`, `value` FROM `settings`;");

    while(query->next())
    {
        i = fieldToPropertyId("settings." + query->value(0).toString());
        if(i < 0)
            continue;

        metaObject()->property(i).write(this, query->value(1));
    }

    delete query;
}

void SComSettings::loadFromJson()
{
    int i = 0;

    i = metaObject()->indexOfProperty("repairDispatcherUsers");
    metaObject()->property(i++).write(this, repairDispatcherUsersJson.Users.join(','));
    metaObject()->property(i++).write(this, repairDispatcherAssignCriteriaJson.DayLimit);
    metaObject()->property(i++).write(this, repairDispatcherAssignCriteriaJson.StatusList.join(','));

    i = metaObject()->indexOfProperty("emailServer");
    metaObject()->property(i++).write(this, emailConfigJson.Host);
    metaObject()->property(i++).write(this, emailConfigJson.Port);
    metaObject()->property(i++).write(this, emailConfigJson.Login);
    metaObject()->property(i++).write(this, emailConfigJson.Password);
    metaObject()->property(i++).write(this, emailConfigJson.Timeout);
    metaObject()->property(i++).write(this, emailConfigJson.EnableSsl);
    metaObject()->property(i++).write(this, emailConfigJson.EnableImplicitSsl);
    metaObject()->property(i++).write(this, emailConfigJson.Template);

    i = metaObject()->indexOfProperty("smsProvider");
    metaObject()->property(i++).write(this, smsConfigJson.Provider);
    metaObject()->property(i++).write(this, smsConfigJson.AuthType);
    metaObject()->property(i++).write(this, smsConfigJson.ApiId);
    metaObject()->property(i++).write(this, smsConfigJson.Login);
    metaObject()->property(i++).write(this, smsConfigJson.Password);
    metaObject()->property(i++).write(this, smsConfigJson.Sender);
}

QWidget* SComSettings::widget(const int propId, const WidgetType type)
{
    QString wName = metaObject()->property(propId + metaObject()->propertyOffset()).name();
    QWidget *w;
    if(type == WidgetType::Label)
    {
        w = i_labelWidgets.value(wName);
    }
    else
    {
        w = i_editorWidgets.value(wName);
    }

    return w;
}

int SComSettings::propertyGroup(const int propId)
{
    QString wName = metaObject()->property(propId + metaObject()->propertyOffset()).name();
    return i_propertyGroup.value(wName);
}

int SComSettings::propertyId(const char *name) const
{
    return (metaObject()->indexOfProperty(name) - metaObject()->propertyOffset());
}

void SComSettings::deleteWidgets()
{
    QWidget *w;
    QString k;
    while(i_labelWidgets.count())
    {
        w = i_labelWidgets.last();
        k = i_labelWidgets.lastKey();
        i_labelWidgets.remove(k);
        delete w;
    }

    while(i_editorWidgets.count())
    {
        w = i_editorWidgets.last();
        k = i_editorWidgets.lastKey();
        i_editorWidgets.remove(k);
        delete w;
    }
}

int SComSettings::count()
{
    return metaObject()->propertyCount() - metaObject()->propertyOffset();
}

// добавление изменённых значений в формате JSON
void SComSettings::sortFieldsByTable(Table table)
{
    for(int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); i++)
    {
        QString field = i_fieldNames.value(metaObject()->property(i).name());
        if(field.startsWith("settings") != table)
            continue;

        if(table)   // Table::Settings
            field = field.replace("settings.", "");

        QVariant val = i_fieldModified.value(metaObject()->property(i).name());
        if(!val.isValid())
            val = i_jsonFieldModified.value(metaObject()->property(i).name());
        if(!val.isValid())
            continue;

        i_valuesMap.insert(field, val);
    }
}

void SComSettings::save()
{
    qDebug().nospace() << "[" << this << "] save()";
    bool nErr = 1;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));
    QString q;

    for(int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); i++)
    {
        metaObject()->property(i).read(this);
    }
    updateJson();

    i_valuesMap.clear();
    sortFieldsByTable(Table::Config);

    fieldsUpdFormatter();
    if(!i_valuesMap.isEmpty())
    {
        q = QString("UPDATE\n  `config`\nSET\n  %2\nWHERE `id` = 1;").arg(fields.join(",\n  "));
        QUERY_EXEC(query, nErr)(q);
    }
    if(!nErr)
        throw 1;

    saveToTableSettings();

    i_fieldModified.clear();
    i_jsonFieldModified.clear();
    delete query;
}

void SComSettings::saveToTableSettings()
{
    bool nErr = 1;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));

    i_valuesMap.clear();
    sortFieldsByTable(Table::Settings);

    query->prepare(QString("UPDATE `settings` SET `value` = :value WHERE `name` = :name"));
    QMap<QString, QVariant>::const_iterator i = i_valuesMap.constBegin();
    while(i != i_valuesMap.constEnd())
    {
        query->bindValue(":name", i.key());
        query->bindValue(":value", i.value());
        nErr = query->exec();
        if(!nErr)
        {
            errorToLog(metaObject()->className(), query->lastError().text());
            throw 1;
        }
        i++;
    }

    delete query;
}

// json объекты нужно обработать отдельно.
// И АСЦ и QSerializer нормально обрабатывают список значений из строк (каждое в кавычках) и из чисел (без кавычек)
// но QSerializer требует название элемента, а АСЦ при указании этого названия сбоит.
void SComSettings::updateJson()
{
    QString dbValue = repairDispatcherUsersJson.Users.join(',');   // старое значение
    if(dbValue.compare(repairDispatcherUsers) != 0) // сравнение старого значения и нового из виджета
    {
//        repairDispatcherUsersJson.fromJsonArray(QString("[%1]").arg(repairDispatcherUsers).toLocal8Bit());
        repairDispatcherUsersJson.Users = repairDispatcherUsers.split(',');
        i_jsonFieldModified.insert("repairDispatcherUsersJson", QString("[%1]").arg(repairDispatcherUsers));
    }

    dbValue = repairDispatcherAssignCriteriaJson.StatusList.join(',');   // старое значение
    if(dbValue.compare(repairDispatcherStatuses) != 0 || repairDispatcherAssignCriteriaJson.DayLimit != repairDispatcherDayLimit) // сравнение старого значения и нового из виджета
    {
        repairDispatcherAssignCriteriaJson.StatusList = repairDispatcherStatuses.split(',');
        repairDispatcherAssignCriteriaJson.DayLimit = repairDispatcherDayLimit;
        i_jsonFieldModified.insert("repairDispatcherAssignCriteriaJson", repairDispatcherAssignCriteriaJson.toRawJson());
    }

/*DBG*/ emailServer = "127.0.0.1";
    if( emailConfigJson.Host != emailServer || emailConfigJson.Port != emailPort ||
        emailConfigJson.Login != emailLogin || emailConfigJson.Password != emailPassword ||
        emailConfigJson.Timeout != emailTimeout || emailConfigJson.EnableSsl != emailEnableSsl ||
        emailConfigJson.EnableImplicitSsl != emailEnableImplicitSsl || emailConfigJson.Template != emailTemplate )
    {
        emailConfigJson.Host = emailServer;
        emailConfigJson.Port = emailPort;
        emailConfigJson.Login = emailLogin;
        emailConfigJson.Password = emailPassword;
        emailConfigJson.Timeout = emailTimeout;
        emailConfigJson.EnableSsl = emailEnableSsl;
        emailConfigJson.EnableImplicitSsl = emailEnableImplicitSsl;
        emailConfigJson.Template = emailTemplate;
        i_jsonFieldModified.insert("emailConfigJson", emailConfigJson.toRawJson());
    }

/*DBG*/ smsLogin = "user";
    if( smsConfigJson.Provider != smsProvider || smsConfigJson.AuthType != smsAuthType ||
        smsConfigJson.ApiId != smsApiId || smsConfigJson.Login != smsLogin ||
        smsConfigJson.Password != smsPassword || smsConfigJson.Sender != smsSender )
    {
        smsConfigJson.Provider = smsProvider;
        smsConfigJson.AuthType = smsAuthType;
        smsConfigJson.ApiId = smsApiId;
        smsConfigJson.Login = smsLogin;
        smsConfigJson.Password = smsPassword;
        smsConfigJson.Sender = smsSender;
        i_jsonFieldModified.insert("smsConfigJson", smsConfigJson.toRawJson());
    }
}

void SComSettings::translate()
{
    tr("editGlobalSettings");
}