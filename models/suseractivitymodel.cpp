#include "suseractivitymodel.h"
#include "global.h"

SUserActivityModel::SUserActivityModel(QObject *parent) : SDatabaseRecord(parent)
{
    i_obligatoryFields << "user_id" << "datetime_" << "notes";
    i_tableName = "users_activity";
}

SUserActivityModel::~SUserActivityModel()
{
}

int SUserActivityModel::id()
{
    return i_id;
}

void SUserActivityModel::setId(const int id)
{
    i_id = id;
}

int SUserActivityModel::userId()
{
    return m_userId;
}

void SUserActivityModel::setUserId(const int user_id)
{
    i_valuesMap.insert("user_id", user_id);
}

QDateTime SUserActivityModel::datetime()
{
    return m_datetime;
}

void SUserActivityModel::setDatetime(const QDateTime datetime)
{
    i_valuesMap.insert("datetime_", datetime);
}

QString SUserActivityModel::address()
{
    return m_address;
}

void SUserActivityModel::setAddress(const QString address)
{
    i_valuesMap.insert("address", address);
}

QString SUserActivityModel::notes()
{
    return m_notes;
}

void SUserActivityModel::setNotes(const QString notes)
{
    i_valuesMap.insert("notes", notes);
}

QString SUserActivityModel::appVersion()
{
    return m_appVersion;
}

void SUserActivityModel::setAppVersion(const QString app_version)
{
    i_valuesMap.insert("app_version", app_version);
}

QString SUserActivityModel::machineName()
{
    return m_machineName;
}

void SUserActivityModel::setMachineName(const QString machine_name)
{
    i_valuesMap.insert("machine_name", machine_name);
}

void SUserActivityModel::appendRecord(const QString &notes, const bool standalone)
{
    setNotes(notes);
    if(standalone)
    {
        i_query->exec(QUERY_BEGIN);
    }
    QUERY_EXEC(i_query, i_nErr)(QUERY_UPD_LAST_USER_ACTIVITY(userDbData->id));
    commit();
    if(standalone)
    {
        QUERY_COMMIT_ROLLBACK(i_query, i_nErr);
    }
    if(i_nErr)
        setId(0);
}

void SUserActivityModel::updateActivityTimestamp()
{
    i_query->exec(QUERY_BEGIN);
    QUERY_EXEC(i_query, i_nErr)(QUERY_UPD_LAST_USER_ACTIVITY(userDbData->id));
    QUERY_COMMIT_ROLLBACK(i_query, i_nErr);
}

void SUserActivityModel::updateLoginTimestamp()
{
    i_query->exec(QUERY_BEGIN);
    QUERY_EXEC(i_query, i_nErr)(QUERY_UPD_LAST_USER_LOGIN(userDbData->id));
    QUERY_COMMIT_ROLLBACK(i_query, i_nErr);

}

bool SUserActivityModel::commit()
{
    if(i_id)
    {
        update();
    }
    else
    {
        setUserId(userDbData->id);
        setDatetime(QDateTime::currentDateTime());
        setAppVersion(APP_VER);
        setMachineName(QHostInfo::localHostName());
        insert();
    }

    return i_nErr;
}
