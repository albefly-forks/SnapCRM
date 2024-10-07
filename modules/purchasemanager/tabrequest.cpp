#include "modules/purchasemanager/tabrequest.h"
#include "ui_tabrequest.h"
#include "modules/purchasemanager/tabmanager.h"

QMap<int, tabPartRequest*> tabPartRequest::p_instance;

tabPartRequest *tabPartRequest::getInstance(int id, MainWindow *parent)
{
    if( !p_instance.contains(id) )
      p_instance.insert(id, new tabPartRequest(id, parent));
    return p_instance.value(id);
}

tabPartRequest::tabPartRequest(int id, MainWindow *parent) :
    tabCommon(parent),
    ui(new Ui::tabPartRequest),
    m_initialRequestId(id),
    m_requestId(id)
{
    ui->setupUi(this);

    m_partRequest = new SPartRequest(this);
    m_clientModel = new SClientModel();
    m_dealerModel = new SClientModel();
    connect(m_dealerModel, &SClientModel::modelUpdated, this, [=]{ui->widgetSuppliers->addSupplierRecord(m_dealerModel->id());});

    initWidgets();

    if(id)
        load(id);

    logUserActivity();

#ifdef QT_DEBUG
    if(!id)
    {
        test_scheduler->start(1000);
    }
#endif
}

tabPartRequest::~tabPartRequest()
{
    delete ui;
    p_instance.remove(m_requestId);   // Обязательно блять!
    if(m_clientModel)
        delete m_clientModel;
    if(m_dealerModel)
        delete m_dealerModel;
}

QString tabPartRequest::tabTitle()
{
    if(m_requestId)
        return tr("Заявка на закупку %1").arg(m_requestId);

    return tr("Новая заявка на закупку");
}

bool tabPartRequest::tabCloseRequest()
{
    try
    {
        if(m_partRequest->isDirty())    throw 1;
        if(ui->lineEditName->text().compare(m_partRequest->name()))    throw 2;
        if(ui->widgetSuppliers->isDirty())     throw 3;
        if(ui->textEditNotes->toPlainText().compare(m_partRequest->notes()))    throw 4;
        if(m_partRequest->id())
        {
            if(m_partRequest->dealer() != m_dealerModel->id())  throw 5;
            foreach (auto mgr, m_partRequest->managers())
            {
                // Право использовать менеджер закупок может быть отозвано и некоторые заявки всегда будут считаться изменёнными
                if(partRequestManagersModel->rowByDatabaseID(mgr) == -1)
                    continue;

                if(!ui->comboBoxManagers->checked().contains(mgr))
                    throw 6;
            }
            foreach (auto mgr, ui->comboBoxManagers->checked())
            {
                if(!m_partRequest->managers().contains(mgr)) throw 7;
            }
            if(m_partRequest->planEndDate() != ui->dateEditPlanEndDate->date()) throw 8;
            if(m_partRequest->state() != partRequestStatesListModel->databaseIDByRow(ui->comboBoxState->currentIndex()))    throw 9;
            if(m_partRequest->priority() != partRequestPrioritiesListModel->databaseIDByRow(ui->comboBoxPriority->currentIndex()))  throw 10;
            if(ui->lineEditTrack->text().compare(m_partRequest->tracking()))    throw 11;
            if(m_partRequest->amount() != ui->doubleSpinBoxAmount->value()) throw 12;
            if(m_partRequest->count() != ui->spinBoxCount->value()) throw 13;
        }
    }
    catch (int step)
    {
        int result = QMessageBox::question(this,
                                           tr("Данные не сохранены"),
                                           tr("Сохранить?"),
                                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        switch(result)
        {
            case QMessageBox::Yes: return commit(0);
            case QMessageBox::Cancel: return 0;
        }
    }

    return 1;
}

void tabPartRequest::copyRequest(const int id)
{
    SPartRequest *old = new SPartRequest;
    old->load(id);
    ui->comboBoxManagers->resetChecked();
    ui->comboBoxManagers->setChecked(old->managers());
    ui->lineEditName->setText(old->name());
    m_dealerModel->blockSignals(true);
    ui->widgetDealerInput->fillCreds(old->dealer());
    m_dealerModel->blockSignals(false);
    if(old->client())
        ui->widgetClientInput->fillCreds(old->client());
    ui->spinBoxCount->setValue(old->count());
    ui->widgetSuppliers->setPredefSupplierId(old->dealer());
    ui->widgetSuppliers->copyLinks(id);

    delete old;
}

void tabPartRequest::setRepair(const int id)
{
    m_partRequest->setRepair(id);
    updateWidgets();
}

void tabPartRequest::setClient(const int id)
{
    m_partRequest->setClient(id);
    updateWidgets();
}

void tabPartRequest::load(const int id)
{
    updateTabPtr(m_initialRequestId, id);
    m_requestId = id;
    m_partRequest->load(id);
    ui->widgetDealerInput->fillCreds(m_partRequest->dealer());
    ui->widgetDealerInput->setHandlingButtons(m_dealerRO);
    ui->widgetSuppliers->select(id);
    ui->widgetComments->load(id);
    emit updateTabTitle(this);
}

void tabPartRequest::initWidgets()
{
    ui->comboBoxCreator->setModel(usersModel);
    ui->comboBoxCreator->setCurrentIndex(usersModel->rowByDatabaseID(m_partRequest->employee()));
    ui->comboBoxCreator->setEnabled(false);
    ui->comboBoxManagers->setModel(partRequestManagersModel);
    ui->comboBoxManagers->showSelectAllRow(true);
    ui->comboBoxManagers->applyGuiSettings();
    ui->dateEditBeginDate->setAllowFutureDate(true);
    ui->dateEditBeginDate->setDateTrackingEnabled(true);
    ui->dateEditPlanEndDate->clear();
    ui->dateEditPlanEndDate->setAllowFutureDate(true);
    ui->dateEditEndDate->clear();
    ui->dateEditEndDate->setAllowBackDate(true);
    ui->dateEditEndDate->setVisible(m_endDateVisible);

    ui->comboBoxState->setModel(partRequestStatesListModel);
    ui->comboBoxState->hideRow(partRequestStatesListModel->rowByDatabaseID(SPartRequest::NewUncommitted));

    ui->comboBoxPriority->setModel(partRequestPrioritiesListModel);
    ui->comboBoxPriority->setCurrentIndex(SPartRequest::Priority::Regular - 1);

//    ui->lineEditName->setButtons("Search"); // когда будет готова вкладка Товары
//    ui->lineEditRepair->setButtons("Search");
    ui->spinBoxCount->setMinimum(1);
    ui->spinBoxCount->setMaximum(99999);
    connect(ui->pushButtonRepair, &QPushButton::clicked, this, [=]{MainWindow::getInstance()->createTabRepair(m_partRequest->repair());});
    connect(ui->pushButtonCreate, &QPushButton::clicked, this, &tabPartRequest::create);
    connect(ui->pushButtonCreateMore, &QPushButton::clicked, this, &tabPartRequest::createAndRepeat);
    connect(ui->pushButtonCopy, &QPushButton::clicked, this, &tabPartRequest::createCopyTab);
    connect(ui->pushButtonSave, &QPushButton::clicked, this, &tabPartRequest::save);

    ui->widgetDealerInput->setClientModel(m_dealerModel);
    ui->widgetDealerInput->setLayoutRefWidget(0, ui->labelName);
    ui->widgetDealerInput->setLayoutRefWidget(1, ui->spinBoxCount);
    ui->widgetDealerInput->setLabelText(tr("Поставщик"));
    ui->widgetDealerInput->setClientsTabTitle(tr("Выбрать поставщика"));
    ui->widgetDealerInput->setBalanceVisible(false);
    ui->widgetDealerInput->setClientsTabDefaultCategory(SClientModel::Categories::Suppliers);

    ui->widgetClientInput->setClientModel(m_clientModel);
    ui->widgetClientInput->setLayoutRefWidget(0, ui->labelName);
    ui->widgetClientInput->setLayoutRefWidget(1, ui->spinBoxCount);
    ui->widgetClientInput->setClientsTabDefaultCategory(SClientModel::Categories::Regulars);
    ui->widgetClientInput->setBalanceVisible(false);

    ui->widgetSuppliers->setSaveButtonVisible(false);   // кнопка Сохранить не отображается никогда, т. к. есть кнопки на нижней панели

    ui->widgetComments->setParentTab(this);
    ui->widgetComments->setMode(SCommentModel::Mode::PartRequest);

    connect(m_partRequest, &SPartRequest::modelReset, this, &tabPartRequest::updateWidgets);
    updateWidgets();
}

void tabPartRequest::setDefaultStyleSheets()
{
    ui->lineEditName->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxCreator->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxManagers->setStyleSheet(commonComboBoxStyleSheet);
    ui->dateEditBeginDate->setStyleSheet(commonDateEditStyleSheet);
    ui->dateEditPlanEndDate->setStyleSheet(commonDateEditStyleSheet);
    ui->dateEditEndDate->setStyleSheet(commonDateEditStyleSheet);
    ui->comboBoxState->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPriority->setStyleSheet(commonComboBoxStyleSheet);
    ui->spinBoxCount->setStyleSheet(commonLineEditStyleSheet);
}

bool tabPartRequest::checkInput()
{
    int error = 0;
    int step = 0;
    setDefaultStyleSheets();

    if ( ++step && ui->comboBoxCreator->currentIndex() < 0 )
    {
        ui->comboBoxCreator->setStyleSheet(commonComboBoxStyleSheetRed);
        error |= 1<<step;
    }
    if ( ++step && ui->comboBoxManagers->checked().count() <= 0 )
    {
        ui->comboBoxManagers->setStyleSheet(commonComboBoxStyleSheetRed);
        error |= 1<<step;
    }
    if( ++step && (m_partRequest->state() == SPartRequest::NewUncommitted && ui->dateEditBeginDate->date().daysTo(ui->dateEditPlanEndDate->date()) < 0) )
    {
        ui->dateEditPlanEndDate->setStyleSheet(commonDateEditStyleSheetRed);
        error |= 1<<step;
    }
    if ( ++step && (!m_newRequest && ui->comboBoxState->currentIndex() < 0) )
    {
        ui->comboBoxState->setStyleSheet(commonComboBoxStyleSheetRed);
        error |= 1<<step;
    }
    if ( ++step && ui->comboBoxPriority->currentIndex() < 0 )
    {
        ui->comboBoxPriority->setStyleSheet(commonComboBoxStyleSheetRed);
        error |= 1<<step;
    }
    if ( ++step && ui->lineEditName->text().isEmpty() )
    {
        ui->lineEditName->setStyleSheet(commonLineEditStyleSheetRed);
        error |= 1<<step;
    }
//    if ( ++step && ui->widgetClientInput->checkInput() )
//    {
//        error |= 1<<step;
//    }
    if ( ++step && ui->spinBoxCount->value() == 0 )
    {
        ui->spinBoxCount->setStyleSheet(commonLineEditStyleSheetRed);
        error |= 1<<step;
    }

    if (error)
    {
        qDebug() << QString("Ошибка создания заявки: возможно, не все обязательные поля заполнены (error 0b%1)").arg(error, 0, 2);
        appLog->appendRecord(QString("Error while creating part request: probably some obligatory fields empty (error 0b%1)").arg(error, 0, 2));
        return 0;
    }
    return 1;
}

void tabPartRequest::setModelData()
{
    QDate plannedEnd = ui->dateEditPlanEndDate->date();
    int newState;

    m_partRequest->setEmployee(usersModel->databaseIDByRow(ui->comboBoxCreator->currentIndex()));
    // Обновление списка ответственных сотрудников должно производиться только после записи данных в БД
    if(m_newRequest)
    {
        newState = SPartRequest::State::Created;
        m_partRequest->setCreated(ui->dateEditBeginDate->dateTime());
    }
    else
    {
        newState = partRequestStatesListModel->databaseIDByRow(ui->comboBoxState->currentIndex());
        m_partRequest->setTracking(ui->lineEditTrack->text());
        m_partRequest->setAmount(ui->doubleSpinBoxAmount->value());
    }

    if(plannedEnd.isValid())
        m_partRequest->setPlanEndDate(plannedEnd);
    m_partRequest->setState(newState);
    m_partRequest->setPriority(partRequestPrioritiesListModel->databaseIDByRow(ui->comboBoxPriority->currentIndex()));

    m_partRequest->setName(ui->lineEditName->text());
    m_partRequest->setDealer(m_dealerModel->id());
    m_partRequest->setClient(m_clientModel->id());
    m_partRequest->setRepair(ui->lineEditRepair->text().toInt());
    m_partRequest->setCount(ui->spinBoxCount->value());
    m_partRequest->setNotes(ui->textEditNotes->toPlainText());

    m_partRequest->setDirty(true);
}

bool tabPartRequest::commit(bool repeatAfter)
{
    if(!checkInput())
        return false;

    bool nErr = 1;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));

    setModelData();

    setDefaultStyleSheets();

    QUERY_LOG_START(metaObject()->className());

    try
    {
        QUERY_EXEC(query,nErr)(QUERY_BEGIN);

        m_partRequest->commit();
        if(m_newRequest)
        {
            m_requestId = m_partRequest->id();
            ui->widgetSuppliers->setRequestId(m_requestId);
        }

        m_partRequest->setManagers(ui->comboBoxManagers->checked());
        m_partRequest->commitManagers();
        ui->widgetSuppliers->saveLinks();
        ui->widgetSuppliers->select(m_requestId);


#ifdef QT_DEBUG
//        throw Global::ThrowType::Debug; // это для отладки (чтобы сессия всегда завершалась ROLLBACK'OM)
#endif
        QUERY_COMMIT_ROLLBACK(query, nErr);
    }
    catch (Global::ThrowType type)
    {
        nErr = 0;
        if(m_newRequest)
        {
            m_requestId = 0;
            m_partRequest->setId(m_requestId);
            m_partRequest->setState(SPartRequest::NewUncommitted);
        }
        if(type == Global::ThrowType::Debug)
        {
            m_partRequest->setDirty(true);
            QString err = "DEBUG ROLLBACK";
            QUERY_ROLLBACK_MSG(query, err);
        }
        else if (type == Global::ThrowType::QueryError)
        {
            QUERY_COMMIT_ROLLBACK_MSG(query, nErr);
        }
        else
            QUERY_COMMIT_ROLLBACK(query, nErr);
    }

#ifdef QT_DEBUG
//    nErr = 1; // и это для отладки (чтобы проверить работу дальше)
#endif

    QUERY_LOG_STOP;

    if (nErr)   // если все запросы выполнены без ошибок
    {
        // TODO: аналогичный код используется в классе tabCashOperation; нужно унифицировать
        if(!repeatAfter)
        {
            updateTabPtr(m_initialRequestId, m_requestId);
            emit updateTabTitle(this);

            ui->widgetClientInput->setHandlingButtons(SClientSelectForm::ViewCard);
            ui->widgetSuppliers->setRequestState(m_partRequest->state());
        }
        else
        {
            m_requestId = m_initialRequestId;
            m_partRequest->deleteLater();
            m_partRequest = new SPartRequest(this);
            ui->widgetSuppliers->clearModel();
            ui->widgetSuppliers->addSupplierRecord(m_dealerModel->id());
        }
        tabPurchaseManager::refreshIfTabExists();
    }

    delete query;

    updateWidgets();

    return nErr;
}

void tabPartRequest::stateHandler(const int state)
{
    m_newRequest = 0;
    m_commentsVisible = 1;
    m_endDateVisible = 0;
    m_requestRO = permissions->enableSparepartRequestManager?0:1;
    m_dealerRO = m_requestRO?SClientSelectForm::ViewCard:SClientSelectForm::Full;
    m_clientRO = SClientSelectForm::ViewCard;
    m_deadlineVisible = true;

    switch (state)
    {
        case SPartRequest::State::NewUncommitted: m_newRequest = 1; m_requestRO = 0; m_dealerRO = SClientSelectForm::Full; m_clientRO = SClientSelectForm::Full; m_commentsVisible = 0; break;
        case SPartRequest::State::Created: break;
        case SPartRequest::State::Cancelled:
        case SPartRequest::State::Finished:
            m_requestRO = 1;
            m_dealerRO = SClientSelectForm::ViewCard;
            m_deadlineVisible &= m_partRequest->planEndDate().isValid();
            m_endDateVisible = 1;
            break;
        default: break;
    }
}

/* Замена ключа в списке singletone-вкладок
*/
void tabPartRequest::updateTabPtr(const int oldId, const int newId)
{
    if(oldId == newId)
        return;

    p_instance.remove(oldId);
    p_instance.insert(newId, this);
}

void tabPartRequest::updateWidgets()
{
    bool clientVisible = m_partRequest->client();
    bool repairVisible = m_partRequest->repair();
    setDefaultStyleSheets();
    stateHandler(m_partRequest->state());

    ui->comboBoxManagers->setChecked(m_partRequest->managers());
    ui->comboBoxManagers->setEnabled(!m_requestRO);


    ui->dateEditBeginDate->setAllowBackDate(!m_newRequest);
    ui->dateEditBeginDate->setDateTime(m_partRequest->createdUtc().toLocalTime());
    ui->dateEditBeginDate->setReadOnly(!m_newRequest);

    ui->labelPlanEndDate->setVisible(m_deadlineVisible);
    ui->dateEditPlanEndDate->setVisible(m_deadlineVisible);
    ui->dateEditPlanEndDate->setAllowBackDate(!m_newRequest);
    if(m_deadlineVisible)
        ui->dateEditPlanEndDate->setDate(m_partRequest->planEndDate());
    ui->dateEditPlanEndDate->setReadOnly(m_requestRO);

    ui->labelEndDate->setVisible(m_endDateVisible);
    ui->dateEditEndDate->setVisible(m_endDateVisible);
    ui->dateEditEndDate->setDate(m_partRequest->endDate());
    ui->dateEditEndDate->setReadOnly(m_requestRO);

    ui->comboBoxState->setCurrentIndex(partRequestStatesListModel->rowByDatabaseID(m_partRequest->state()));
    ui->comboBoxState->setDisabled(m_newRequest || m_requestRO);

    ui->comboBoxPriority->setCurrentIndex(partRequestPrioritiesListModel->rowByDatabaseID(m_partRequest->priority()));
    ui->comboBoxPriority->setEnabled(!m_requestRO);

    ui->labelTrack->setVisible(m_commentsVisible);
    ui->lineEditTrack->setVisible(m_commentsVisible);
    ui->lineEditTrack->setText(m_partRequest->tracking());
    ui->lineEditTrack->setReadOnly(m_requestRO);

    ui->labelAmount->setVisible(m_commentsVisible);
    ui->doubleSpinBoxAmount->setVisible(m_commentsVisible);
    ui->doubleSpinBoxAmount->setValue(m_partRequest->amount());
    ui->doubleSpinBoxAmount->setReadOnly(m_requestRO);

    ui->lineEditName->setText(m_partRequest->name());
    ui->lineEditName->setReadOnly(m_requestRO);

    ui->widgetDealerInput->setHandlingButtons(m_dealerRO);

    ui->widgetClientInput->setVisible(clientVisible);
    if(clientVisible)
    {
        ui->widgetClientInput->fillCreds(m_partRequest->client());
        ui->widgetClientInput->setHandlingButtons(SClientSelectForm::AccessMode::ViewCard);
    }

    ui->labelRepair->setVisible(repairVisible);
    ui->lineEditRepair->setVisible(repairVisible);
    ui->lineEditRepair->setReadOnly(m_requestRO);
    ui->pushButtonRepair->setVisible(repairVisible);
    if(repairVisible)
        ui->lineEditRepair->setText(QString::number(m_partRequest->repair()));

    ui->spinBoxCount->setValue(m_partRequest->count());
    ui->spinBoxCount->setReadOnly(m_requestRO);

    ui->textEditNotes->setText(m_partRequest->notes());
    ui->textEditNotes->setReadOnly(m_requestRO);

    // режим RO может быть автивен, если у пользователя нет права Использовать менеджер; открыть карточку заявки при этом он может из карточки ремонта
    ui->widgetSuppliers->setRequestState(m_requestRO?SPartRequest::Finished:m_partRequest->state());

    ui->widgetComments->setVisible(m_commentsVisible);

    ui->pushButtonCreate->setVisible(!m_requestRO && m_newRequest);
    ui->pushButtonCreateMore->setVisible(!m_requestRO && m_newRequest);
    ui->pushButtonCopy->setVisible(!m_newRequest);
    ui->pushButtonSave->setVisible(!m_requestRO && !m_newRequest);
    ui->pushButtonLog->setVisible(!m_newRequest);
}

void tabPartRequest::create()
{
    commit(0);
}

void tabPartRequest::createAndRepeat()
{
    commit(1);
}

void tabPartRequest::createCopyTab()
{
    MainWindow *mw = MainWindow::getInstance();
    tabPartRequest *newTab = nullptr;

    newTab = static_cast<tabPartRequest*>(mw->createTabPartRequest(0));
    newTab->copyRequest(m_requestId);
}

void tabPartRequest::save()
{
    commit(0);
}

void tabPartRequest::guiFontChanged()
{
    QFont font(guiFont());

    ui->comboBoxManagers->setFont(font);
    ui->comboBoxState->setFont(font);
}

#ifdef QT_DEBUG
void tabPartRequest::randomFill()
{
    int i;

    i = QRandomGenerator::global()->bounded(partRequestPrioritiesListModel->rowCount());
    ui->comboBoxPriority->setCurrentIndex(i);

    if (QRandomGenerator::global()->bounded(100) > 50)  // 50/50
        ui->widgetDealerInput->fillCreds(SClientInputFormBase::randomClientIdFromDB("is_dealer", 1));
    if (QRandomGenerator::global()->bounded(100) > 50)  // 50/50
        ui->widgetClientInput->fillCreds(SClientInputFormBase::randomClientIdFromDB("is_dealer", 0));
    i = QRandomGenerator::global()->bounded(10);
    if(i)
        i *= 5;
    else
        i += 1;
    ui->spinBoxCount->setValue(i);
}

void tabPartRequest::test_scheduler_handler()
{
//    randomFill();
//    ui->widgetDealerInput->setLineEditText("tag");
//    ui->widgetClientInput->setLineEditText("чер");
//    copyRequest(217);
//    load(217);
}
#endif
