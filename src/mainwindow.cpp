#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "issuesmodel.h"
#include "redminerequest.h"
#include "issuesortfilterproxymodel.h"
#include "settingsdialog.h"
#include "globals.h"
#include "redmine.h"
#include "progressdialog.h"
#include "timeconfirmationdialog.h"
#include "about.h"

#include <QtNetwork>
#include <QtCore>
#include <QtDebug>
#include <QtGui>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_currentIssueId(0)
{
    ui->setupUi(this);

    // set default settings
    QCoreApplication::setOrganizationName("Adyax");
    QCoreApplication::setOrganizationDomain("adyax.com");
    QCoreApplication::setApplicationName("qRedmine");

    createWidgets();
    createActions();
//    createTrayIcon();
    createShortcuts();

    // FIXME: comment do to qt5 tray icon bug
//#ifdef Q_OS_LINUX
//    QProcess process;
//    QStringList listIdentifierOfAllowedDistros;
//    listIdentifierOfAllowedDistros << "ksmserver" << "openbox";
//    process.start("pidof", listIdentifierOfAllowedDistros);
//    if (!process.waitForFinished() || !process.readAllStandardOutput().isEmpty()) {
//        m_trayIcon->show();
//    }
//#else
//    m_trayIcon->show();
//#endif

    // check that we have all crucial parameters configured
    readSettings();
    verifySettings();

    loadStyleSheet();

    m_redmine->fetchActivities();
    m_progress_dialog->show();
    fetchIssues();
    m_redmine->fetchTimeEntries();
}

MainWindow::~MainWindow()
{
    delete m_loggedTimer;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // FIXME: all that code doesn't work on os x
    writeSettings();
    qDebug() << "MainWindow::closeEvent called";
//    if (m_trayIcon->isVisible()) {
//        hide();
//    }
//    else {
//        slotQuit();
//    }
    slotQuit();

    event->ignore();
}

// private slots

void MainWindow::slotGetIssuesFinished(const QList<QJsonValue> &issues)
{
    qDebug() << "slotGetMyIssuesFinished count: " << issues.count();

    // getting list of current issues
    QStringList currentIssuesList;
    for (int i = 0; i<m_issuesModel->rowCount(); i++) {
        currentIssuesList << m_issuesModel->index(i, Globals::IdColumn).data().toString();
    }

    QStringList projectsList;

    foreach (QJsonValue i, issues) {
        QJsonObject obj = i.toObject();
        QList<QStandardItem *> row;
        QStandardItem *project = new QStandardItem(obj.value("project").toObject().value("name").toString());
        QStandardItem *id = new QStandardItem(QString::number(obj.value("id").toDouble()));
        QStandardItem *priority = new QStandardItem(obj.value("priority").toObject().value("name").toString());
        QStandardItem *subject = new QStandardItem(obj.value("subject").toString());

        // sort everything alphabetically and only priority by a real priority (from blocker to low)
        project->setData(project->text(), Globals::SortRole);
        id->setData(id->text(), Globals::SortRole);
        priority->setData(ui->comboFilterByPriority->findText(priority->text()), Globals::SortRole);
        subject->setData(subject->text(), Globals::SortRole);

        // adding new projects
        if (!projectsList.contains(project->text())) {
            projectsList << project->text();
        }

        // adding new issues
        if (m_issuesModel->findItems(id->text(), Qt::MatchExactly, Globals::IdColumn).size() == 0) {
            row << project << id << priority << subject;
            m_issuesModel->appendRow(row);
        }
        else {
            currentIssuesList.removeAll(id->text());
        }
    }

    qDebug() << "Removing following issues: " << currentIssuesList;

    removeOldIssues(currentIssuesList);
    updateProjectsFilter(projectsList);

    // sort issues model
    m_proxyIssuesModel->sort(m_proxyIssuesModel->sortColumn());

    // select first element if none is selected
    if (m_proxyIssuesModel->mapToSource(ui->issuesView->currentIndex()).row() == -1) {
        ui->issuesView->setCurrentIndex(m_proxyIssuesModel->index(0, 0));
    }

    m_issuesFetchState = IssueFetchStates::Idle;
    m_progress_dialog->done(0);
    ui->issuesView->setFocus();
    ui->statusBar->showMessage(tr("%n issue(s) fetched.", "", m_issuesModel->rowCount()), 2000);
}

void MainWindow::slotGetIssuesProgress(const int &total, const int &offset)
{
    m_progress_dialog->setMaxValue(total);
    m_progress_dialog->setValue(offset);
}

void MainWindow::slotFetchActivitiesFinished()
{

}

void MainWindow::slotFetchTimeEntriesFinished(const QList<QJsonValue> &entries)
{
    qDebug() << "slotFetchTimeEntriesFinished count: " << entries.count();

    double hours = 0;
    foreach (QJsonValue entry, entries) {
        hours += entry.toObject().value("hours").toDouble();
    }

    qDebug() << "hours: " << hours;

    QSettings settings;

    ui->loggedTimeLabel->setText(tr("%1/%2h").arg(hours).arg(settings.value("per-month hours goal", PER_MONTH_HOURS_GOAL).toString()));

    ui->loggedTimeDisplayLabel->setVisible(true);
    ui->loggedTimeLabel->setVisible(true);
}

void MainWindow::slotPostTimeEntrySuccess()
{
    m_progress_dialog->done(0);
    ui->statusBar->showMessage(tr("Time logged"), 2000);
    m_redmine->fetchTimeEntries();
//    fetchIssues();
}

void MainWindow::slotTimerTimeout()
{
    qint64 msecs = m_loggedTimer->elapsed();
    int hours = msecs/(1000*60*60);
    int minutes = (msecs-(hours*1000*60*60))/(1000*60);
    double time = (double)hours+(double)minutes*100/60/100;
    QString timeString = QString("%1h%2m (%3h)").arg(hours, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0')).arg(QString::number(time, 'f', 2));

    ui->timeLabel->setText(timeString);
    ui->recordIndicator->setVisible(!ui->recordIndicator->isVisible());
    m_activeIssueTimeAction->setText(QString("#%1: %2").arg(currentIssueId()).arg(timeString));

    if (currentIssueIndex().row() != -1) {
        QString subject = m_issuesModel->item(currentIssueIndex().row(), Globals::SubjectColumn)->text();
        QString project = m_issuesModel->item(currentIssueIndex().row(), Globals::ProjectColumn)->text();

//        m_trayIcon->setToolTip(QString("%1 #%2 - %3: %4")
//                               .arg(project)
//                               .arg(currentIssueId())
//                               .arg(subject)
//                               .arg(timeString));
    }
    else {
//        m_trayIcon->setToolTip(APP_NAME);
    }
}

void MainWindow::slotOpenSettingsDialog()
{
    SettingsDialog sd(this);
    if (sd.exec() == QDialog::Accepted) {
        // stop timer and fetch issues
        if (currentIssueId() != 0) {
            QMessageBox::warning(this, tr("Time logging will be stopped"), tr("In order to apply latest settings we are going to stop logging time for your current issue and send time logs to redmine."));
            stopLogging();
        }
        else {
            fetchIssues();
        }
    }
}

void MainWindow::slotTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
     case QSystemTrayIcon::Trigger:
        slotToggleVisibility();
        break;
    // getting rid of compiler warnings
    default:
        break;
    }
}

/**
 * @brief MainWindow::slotToggleVisibility
 *  Toggles visibility of the window.
 */
void MainWindow::slotToggleVisibility()
{
    if (isMinimized()) {
        qDebug() << "is minimized";
        hide();
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        show();
    }
    else if (isActiveWindow()) {
        hide();
        m_restoreMinimizeAction->setText(tr("&Restore"));

    }
    else if (!isVisible()) {
        show();
        activateWindow();
        m_restoreMinimizeAction->setText(tr("&Minimize"));
    }
    else {
        activateWindow();
        raise();
        m_restoreMinimizeAction->setText(tr("&Minimize"));
    }
}

/**
 * @brief MainWindow::slotShowComboboxPopup
 *  Used to show combobox of the filter.
 */
void MainWindow::slotShowComboboxPopup(QWidget *combo)
{
    QComboBox *c = qobject_cast<QComboBox *>(combo);
    c->showPopup();
}

/**
 * @brief MainWindow::slotStartStopLogging
 *  Single slot to stop logging and send time entry and start new logging immediately.
 */
void MainWindow::slotStartStopLogging()
{
    qDebug() << "double clicked";

    int cid = currentIssueId();

    stopLogging();
    if (cid != selectedIssueId())
        startLogging();
}

void MainWindow::slotOpenCurrentIssueUrl()
{
    if (currentIssueId() > 0) {
        QSettings settings;
        QString issueURL = QString("%1/issues/%2").arg(settings.value("redmine connection/url").toString()).arg(currentIssueId());
        QDesktopServices::openUrl(issueURL);
    }
}

void MainWindow::slotOpenSelectedIssueUrl()
{
    if (selectedIssueId() != -1) {
        QSettings settings;
        QString issueURL = QString("%1/issues/%2").arg(settings.value("redmine connection/url").toString()).arg(selectedIssueId());
        QDesktopServices::openUrl(issueURL);
    }
}

void MainWindow::slotAbout()
{
    About about;
    about.exec();
}

void MainWindow::slotQuit()
{
    if (currentIssueId() != 0) {
//        if (!this->isVisible())
//            slotToggleVisibility();

        int result = QMessageBox::warning(
                    this,
                    tr("Do you really want to quit?"),
                    tr("Time logging is currently active, if you will exit the application without logging it - timelog won't be sent to Redmine.\n\nDo you really want to exit the application without logging time?"),
                    QMessageBox::Yes, QMessageBox::Cancel);
        if (result == QMessageBox::Cancel)
            return;
    }

    qDebug() << "MainWindow::slotQuit called";
    writeSettings();
    qApp->quit();
}

void MainWindow::on_comboFilterByProject_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "All")
        m_proxyIssuesModel->setFilterProject("");
    else
        m_proxyIssuesModel->setFilterProject(arg1);
}

void MainWindow::on_comboFilterByPriority_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "All")
        m_proxyIssuesModel->setFilterPriority("");
    else
        m_proxyIssuesModel->setFilterPriority(arg1);
}

void MainWindow::on_issueIdEdit_textChanged(const QString &arg1)
{
    m_proxyIssuesModel->setFilterId(arg1);
}

void MainWindow::on_issueSubjectEdit_textChanged(const QString &arg1)
{
    m_proxyIssuesModel->setFilterSubject(arg1);
}

void MainWindow::on_startStopLogButton_clicked(bool checked)
{
    if (checked == true)
        startLogging();
    else
        stopLogging();
}

void MainWindow::createWidgets()
{
    // will use signal mapper for keyboard shortcuts
    m_signalMapper = new QSignalMapper(this);

    // issue timelog counter
    m_loggedTimer = new QElapsedTimer();
    m_timeLabelTimer = new QTimer(this);
    connect(m_timeLabelTimer, SIGNAL(timeout()), this, SLOT(slotTimerTimeout()));

    // issue refresh timer
    m_refreshTimer = new QTimer(this);
    // TODO: move that to settings
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::fetchIssues);
    m_refreshTimer->start(1000*60*15);

    QSettings settings;
    m_redmine = new Redmine(settings.value("redmine connection/url").toString(), settings.value("redmine connection/api_key").toString(), this);

    if (settings.value("redmine connection/basic_auth_enabled", false).toBool() == true) {
        QString ba_login = settings.value("redmine connection/basic_auth_login", "").toString();
        QString ba_pass = settings.value("redmine connection/basic_auth_pass", "").toString();

        m_redmine->setBasicAuth(ba_login, ba_pass);
        m_redmine->setBasicAuthEnabled(true);
    }
    connect(m_redmine, SIGNAL(issuesFetched(QList<QJsonValue>)), this, SLOT(slotGetIssuesFinished(QList<QJsonValue>)));
    connect(m_redmine, SIGNAL(postTimeEntrySuccess()), this, SLOT(slotPostTimeEntrySuccess()));
    connect(m_redmine, SIGNAL(timeEntriesFetched(QList<QJsonValue>)), this, SLOT(slotFetchTimeEntriesFinished(QList<QJsonValue>)));

    // set of issues/projects/... models
    m_proxyIssuesModel = new IssueSortFilterProxyModel(this);
    m_issuesModel = new IssuesModel(0, 4, this);
    m_issuesModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Project"));
    m_issuesModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID"));
    m_issuesModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Priority"));
    m_issuesModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Subject"));

    m_proxyIssuesModel->setSourceModel(m_issuesModel);
    m_proxyIssuesModel->setSortRole(Globals::SortRole);
    m_proxyIssuesModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyIssuesModel->setDynamicSortFilter(false);
    ui->issuesView->setModel(m_proxyIssuesModel);
    ui->issuesView->header()->setDefaultAlignment(Qt::AlignCenter);
//    ui->issuesView->setColumnHidden(0, true);

    m_projectsModel = new QStringListModel(this);
    ui->comboFilterByProject->setModel(m_projectsModel);

    // various labels
    m_issuesLoaderLabel = new QLabel(this);
    QMovie *loaderMovie = new QMovie(this);
    loaderMovie->setFileName(":/images/images/loader.gif");
    m_issuesLoaderLabel->setMovie(loaderMovie);
    ui->verticalLayout->addWidget(m_issuesLoaderLabel);
    ui->statusBar->addPermanentWidget(m_issuesLoaderLabel);

    // setting default focus
    ui->issuesView->setFocus();

    m_progress_dialog = new ProgressDialog(this);
    connect(m_redmine, SIGNAL(issuesFetchProgress(int,int)), this, SLOT(slotGetIssuesProgress(int,int)));

    setRecordLabelsVisible(false);

    connect(ui->assignedToMeRadioButton, &QRadioButton::toggled, this, &MainWindow::fetchIssues);
    connect(ui->assignedWatchingRadioButton, &QRadioButton::toggled, this, &MainWindow::fetchIssues);
    connect(ui->allIssuesRadioButton, &QRadioButton::toggled, this, &MainWindow::fetchIssues);

    ui->loggedTimeDisplayLabel->hide();
    ui->loggedTimeLabel->hide();
}

// private methods

void MainWindow::createActions()
{
    m_activeIssueAction = new QAction(APP_NAME, this);
    m_activeIssueAction->setEnabled(false);
    connect(m_activeIssueAction, SIGNAL(triggered()), this, SLOT(slotOpenCurrentIssueUrl()));

    m_activeIssueTimeAction = new QAction(tr("00h00m (0.00h)"), this);
    m_activeIssueTimeAction->setEnabled(false);

    m_refreshAction = new QAction(tr("Refresh issues"), this);
    connect(m_refreshAction, &QAction::triggered, this, &MainWindow::fetchIssues);

    m_restoreMinimizeAction = new QAction(tr("&Minimize"), this);
    connect(m_restoreMinimizeAction, SIGNAL(triggered()), this, SLOT(slotToggleVisibility()));

    m_settingsAction = new QAction(tr("&Settings"), this);
    connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(slotOpenSettingsDialog()));
    connect(ui->actionPreferences, SIGNAL(triggered()), this, SLOT(slotOpenSettingsDialog()));

    m_quitAction = new QAction(tr("&Quit"), this);
    connect(m_quitAction, SIGNAL(triggered()), this, SLOT(slotQuit()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(slotQuit()));

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));
}

void MainWindow::createTrayIcon()
{
    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction(m_activeIssueAction);
    m_trayIconMenu->addAction(m_activeIssueTimeAction);
    m_trayIconMenu->addSeparator();

    m_trayIconMenu->addAction(m_refreshAction);
    m_trayIconMenu->addSeparator();

    m_trayIconMenu->addAction(m_restoreMinimizeAction);
    m_trayIconMenu->addAction(m_settingsAction);
    m_trayIconMenu->addSeparator();

    m_trayIconMenu->addAction(m_quitAction);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(APP_NAME);

    QByteArray category = qgetenv("SNI_CATEGORY");
    if (!category.isEmpty()) {
        m_trayIcon->setProperty("_qt_sni_category", QString::fromLocal8Bit(category));
    }

    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->setIcon(QIcon(":/images/images/redmine_icon.png"));

    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(slotTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

void MainWindow::createShortcuts()
{
    m_quitShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
    connect(m_quitShortcut, SIGNAL(activated()), this, SLOT(slotQuit()));

    m_focusIssueView = new QShortcut(QKeySequence(tr("Esc")), this);
    connect(m_focusIssueView, SIGNAL(activated()), ui->issuesView, SLOT(setFocus()));

    m_focusFilterPriority = new QShortcut(QKeySequence(tr("Alt+R")), this);
    connect(m_focusFilterPriority, SIGNAL(activated()), m_signalMapper, SLOT(map()));
    m_signalMapper->setMapping(m_focusFilterPriority, ui->comboFilterByPriority);

    m_focusFilterProject = new QShortcut(QKeySequence(tr("Alt+P")), this);
    connect(m_focusFilterProject, SIGNAL(activated()), m_signalMapper, SLOT(map()));
    m_signalMapper->setMapping(m_focusFilterProject, ui->comboFilterByProject);

    m_focusFilterIssue = new QShortcut(QKeySequence(tr("Alt+I")), this);
    connect(m_focusFilterIssue, SIGNAL(activated()), ui->issueIdEdit, SLOT(setFocus()));

    m_focusFilterSubject = new QShortcut(QKeySequence(tr("Alt+S")), this);
    connect(m_focusFilterSubject, SIGNAL(activated()), ui->issueSubjectEdit, SLOT(setFocus()));

    connect(m_signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(slotShowComboboxPopup(QWidget*)));

    //    m_issuesViewKeyDown = new QShortcut(QKeySequence(tr("j")), this);
    //    connect(m_issuesViewKeyDown, SIGNAL(activated()), ui->issuesView, SLOT());

    m_issuesViewCtrlEnter = new QShortcut(QKeySequence(Qt::Key_Return), this);
    QShortcut *issuesViewReturn = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    connect(m_issuesViewCtrlEnter, SIGNAL(activated()), this, SLOT(slotStartStopLogging()));
    connect(ui->issuesView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotStartStopLogging()));
    connect(issuesViewReturn, SIGNAL(activated()), this, SLOT(slotStartStopLogging()));

    QShortcut *refresh = new QShortcut(QKeySequence(tr("F5")), this);
    connect(refresh, &QShortcut::activated, this, &MainWindow::fetchIssues);

    QShortcut *refreshR = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    connect(refreshR, &QShortcut::activated, this, &MainWindow::fetchIssues);

    QShortcut *closeWindow = new QShortcut(QKeySequence(tr("Ctrl+W")), this);
    connect(closeWindow, SIGNAL(activated()), this, SLOT(close()));

    QShortcut *issuesViewOpenIssueURL = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O), this);
    connect(issuesViewOpenIssueURL, SIGNAL(activated()), this, SLOT(slotOpenSelectedIssueUrl()));
}

void MainWindow::verifySettings()
{
    // verify settings
    QSettings settings;
    settings.beginGroup("redmine connection");
    if (settings.value("url", "").toString().isEmpty() || settings.value("api_key", "").toString().isEmpty()) {
        SettingsDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            // normally this should not happen as SettingsDialog prevents rejecting without settings configuration options
            QMessageBox::critical(this, "Configuration error", "This program cannot operate without correct configuration of Redmine URL and API key", QMessageBox::Ok);
        }
    }
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("headerSizeProject", ui->issuesView->columnWidth(Globals::ProjectColumn));
    settings.setValue("headerSizeId", ui->issuesView->columnWidth(Globals::IdColumn));
    settings.setValue("headerSizePriority", ui->issuesView->columnWidth(Globals::PriorityColumn));
    settings.setValue("headerSizeSubject", ui->issuesView->columnWidth(Globals::SubjectColumn));
    settings.setValue("geometry", saveGeometry());
    settings.endGroup();

    if (ui->saveFiltersToolButton->isChecked()) {
        settings.beginGroup("filters");
        settings.setValue("saveFilterState", true);

        settings.setValue("issueId", ui->issueIdEdit->text());
        settings.setValue("issueSubject", ui->issueSubjectEdit->text());

        settings.setValue("assignedToMe", ui->assignedToMeRadioButton->isChecked());
        settings.setValue("assignedWatching", ui->assignedWatchingRadioButton->isChecked());
        settings.setValue("allIssues", ui->allIssuesRadioButton->isChecked());
        settings.endGroup();
    }
}

void MainWindow::readSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    ui->issuesView->setColumnWidth(Globals::ProjectColumn, settings.value("headerSizeProject", ui->issuesView->columnWidth(Globals::ProjectColumn)).toInt());
    ui->issuesView->setColumnWidth(Globals::IdColumn, settings.value("headerSizeId", ui->issuesView->columnWidth(Globals::IdColumn)).toInt());
    ui->issuesView->setColumnWidth(Globals::PriorityColumn, settings.value("headerSizePriority", ui->issuesView->columnWidth(Globals::PriorityColumn)).toInt());
    ui->issuesView->setColumnWidth(Globals::SubjectColumn, 100);
    restoreGeometry(settings.value("geometry").toByteArray());
    settings.endGroup();

    settings.beginGroup("filters");
    if (settings.value("saveFilterState").toBool()) {
        ui->saveFiltersToolButton->setChecked(true);

        ui->issueSubjectEdit->insert(settings.value("issueSubject", "").toString());
        ui->issueIdEdit->insert(settings.value("issueId", "").toString());

        ui->assignedToMeRadioButton->setChecked(settings.value("assignedToMe").toBool());
        ui->assignedWatchingRadioButton->setChecked(settings.value("assignedWatching").toBool());
        ui->allIssuesRadioButton->setChecked(settings.value("allIssues").toBool());
    }
    settings.endGroup();
}

void MainWindow::loadStyleSheet()
{
    QFile file(":/data/style.css");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "cannot open stylesheet file";
        return;
    }

    QString contents(file.readAll());

    // currently we set styles only for treeview
    QPalette p(ui->issuesView->palette());
    QColor alt = p.color(QPalette::AlternateBase);
    alt.setAlpha(180);
    contents.replace("%palette-alternate-background-color", QString("rgba(%1,%2,%3,%4)").arg(alt.red()).arg(alt.green()).arg(alt.blue()).arg(alt.alpha()));

    QColor bg_alt = p.color(QPalette::Base);
    contents.replace("%palette-background-color", QString("rgb(%1,%2,%3)").arg(bg_alt.red()).arg(bg_alt.green()).arg(bg_alt.blue()));

    ui->issuesView->setStyleSheet(contents);
}

/**
 * @brief MainWindow::setHighlightCurrentItem
 *  Highlight currently logged issue in the issues table.
 */
void MainWindow::setHighlightCurrentItem(bool highlight)
{
    QFont font;
    font.setBold(highlight);
    QColor color;

    if (highlight == true)
        color = QColor::fromRgb(106, 168, 79);
    else
        color = QColor::fromRgb(0, 0, 0);

    for (int i = 0; i < m_issuesModel->columnCount(); i++) {
        m_issuesModel->setData(m_issuesModel->index(currentIssueIndex().row(), i), font, Qt::FontRole);
        m_issuesModel->setData(m_issuesModel->index(currentIssueIndex().row(), i), color, Qt::ForegroundRole);
    }
}

void MainWindow::setRecordLabelsVisible(bool visible)
{
    if (!visible)
        ui->issueLinkLabel->setText("");

    ui->timeLabel->setVisible(visible);
    ui->recordIndicator->setVisible(visible);
}

/**
 * @brief MainWindow::startCounter
 *  Start logging time for an issue.
 * @param id
 *  If issue id is passed as a parameter it will be used to log time, otherwise currently selected issue will be used.
 */

void MainWindow::startLogging(int id)
{
    if (selectedIssueId() != -1 && id < 1) {
        // save index of current item
        m_currentIssueId = selectedIssueId();
    }
    else if (id > 0 && !m_issuesModel->findItems(QString::number(id), Qt::MatchExactly, Globals::IdColumn).isEmpty()) {
        // save index of current item
        m_currentIssueId = id;
    }

    // checking if issue id has been set
    if (m_currentIssueId < 1) {
        QMessageBox::warning(this, tr("Cannot log time"), tr("Cannot log time because no issue has been selected."));
        ui->startStopLogButton->setChecked(false);
        return;
    }

    ui->startStopLogButton->setText("Stop logging");
    ui->startStopLogButton->setChecked(true);

    // reset timer and start it again
    m_loggedTimer->restart();
    m_timeLabelTimer->start(1000);

    QSettings settings;
    QString issueURL = QString("%1/issues/%2").arg(settings.value("redmine connection/url").toString()).arg(currentIssueId());
    QString issueLink = QString("<a href='%1'>%2 #%3: %4</a>")
            .arg(issueURL)
            .arg(m_issuesModel->item(currentIssueIndex().row(), Globals::ProjectColumn)->text())
            .arg(currentIssueId())
            .arg(m_issuesModel->item(currentIssueIndex().row(), Globals::SubjectColumn)->text());

    // minor issue title formatting
    QString project = m_issuesModel->item(currentIssueIndex().row(), Globals::ProjectColumn)->text();
    if (project.length() > 5)
        project = project.left(5).append("...");

    QString issueText = QString("%1 - %3")
            .arg(project)
            .arg(m_issuesModel->item(currentIssueIndex().row(), Globals::SubjectColumn)->text());

    // said business school
    if (issueText.length() > 30)
        issueText = issueText.left(30).append("...");

    ui->issueLinkLabel->setText(issueLink);
    ui->timeLabel->setText("00h00m (0.00h)");
    m_activeIssueAction->setText(issueText);
    m_activeIssueAction->setEnabled(true);
    m_activeIssueTimeAction->setText(tr("#%1: 00h00m (0.00h)").arg(currentIssueId()));

    setHighlightCurrentItem(true);
    setRecordLabelsVisible(true);

    ui->statusBar->showMessage(tr("Logging time for issue #%1").arg(currentIssueId()));
}

/**
 * @brief MainWindow::stopLogging
 *  Stop logging time and send a time-entry to Redmine.
 *  FIXME: currently doesn't support saving time-log if there's no internet connection.
 */
void MainWindow::stopLogging()
{
    qDebug() << "stopLogging";
    // make sure that we don't accidentally send an empty log
    if (currentIssueId() != 0) {
        // stop timer and send time logs
        m_timeLabelTimer->stop();

        // FIXME: set some meaningfull tooltip
//        m_trayIcon->setToolTip(APP_NAME);

        ui->startStopLogButton->setText("Start logging");
        ui->startStopLogButton->setChecked(false);

        // update tray menu indicators
        m_activeIssueAction->setText(APP_NAME);
        m_activeIssueAction->setEnabled(false);
        m_activeIssueTimeAction->setText("00h00m (0.00h)");

        // disable logging indicators
        setHighlightCurrentItem(false);
        setRecordLabelsVisible(false);

        // calculate logged time and convert it to double
        qint64 msecs = m_loggedTimer->elapsed();
        double hours = msecs/(1000*60*60);
        double minutes = (msecs-(hours*1000*60*60))/(1000*60);
        double time = hours+minutes*100/60/100;

        qDebug() << "recorded time: " << time;

        // don't send empty timelogs, Redmine will round anything <0.01 to 0.00 anyway

        if (time >= 0.01) {
            // TODO: how to find current project and issue name
            QString subject = m_issuesModel->item(currentIssueIndex().row(), Globals::SubjectColumn)->text();
            QString project = m_issuesModel->item(currentIssueIndex().row(), Globals::ProjectColumn)->text();
            QString issue_link = QString("<a href='%1/issues/%2'>#%3 - %4</a>")
                    .arg(m_redmine->getBaseURL())
                    .arg(currentIssueId())
                    .arg(currentIssueId())
                    .arg(subject);

            QList<Redmine::Activities> activities = m_redmine->getActivities();

            QSettings settings;
            TimeConfirmationDialog dialog(time, issue_link, project, this);
            dialog.setActivitiesComboValues(activities, settings.value("defaultActivity", 0).toInt());
            dialog.exec();

            ui->statusBar->showMessage(tr("Sending timelog for issue #%1").arg(currentIssueId()));
            m_progress_dialog->setLabel(tr("Sending time log..."));
            m_progress_dialog->show();
            m_redmine->postTimeEntry(currentIssueId(), dialog.getTime(), dialog.getActivity());

            // last selected activity is going to be a default one next time
            settings.setValue("defaultActivity", dialog.getActivity());
        }
        else
            ui->statusBar->showMessage(tr("Recorded time is less than 0.01h, won't send it to Redmine."), 4000);

        // clear current index
        m_currentIssueId = 0;
    }
}

void MainWindow::fetchIssues()
{
    if (m_issuesFetchState == IssueFetchStates::Fetching)
        return;

    m_issuesFetchState = IssueFetchStates::Fetching;
    m_redmine->setWatcherEnabled(ui->assignedWatchingRadioButton->isChecked());
    ui->statusBar->showMessage(tr("Fetching issues..."));

    if (this->isActiveWindow()) {
        m_progress_dialog->setLabel(tr("Fetching issues..."));
        m_progress_dialog->show();
    }

    if (ui->allIssuesRadioButton->isChecked()) {
        m_redmine->fetchAllIssues();
    }
    else {
        m_redmine->fetchMyIssues();
    }
}

/**
 * @brief MainWindow::removeOldIssues
 *  Removes items from issue view with values in the list equal to ID column in the view.
 */
void MainWindow::removeOldIssues(const QStringList &list)
{
    QStringListIterator ci(list);
    while (ci.hasNext()) {
        QString id = ci.next();
        m_issuesModel->removeRow(m_issuesModel->findItems(id, Qt::MatchExactly, Globals::IdColumn).first()->row());

//        if (currentIssueId() == id.toInt())
//            m_trayIcon->showMessage(tr("qRedmine warning"), tr("Current issue #%1 has been removed from the list of issues because it has been deleted or reassigned to another person.").arg(id), QSystemTrayIcon::Warning);
    }
}

/**
 * @brief MainWindow::updateProjectsFilter
 *  Updates projects filter with new items. Tries to remember previously selected item.
 */
void MainWindow::updateProjectsFilter(QStringList &projects)
{
    projects.sort();
    projects.prepend("All");

    // remember selected project
    QString currentProject = ui->comboFilterByProject->currentText();
    m_projectsModel->setStringList(projects);

    // restore selected project
    if (projects.contains(currentProject)) {
        ui->comboFilterByProject->setCurrentText(currentProject);
    }
    else {
        ui->comboFilterByProject->setCurrentText("All");
    }
}

/**
 * @brief MainWindow::selectedIssueId
 *  Returns Redmine ID of the currently selected issue in the issues table or -1 if there are no selected issues.
 */
int MainWindow::selectedIssueId() const
{
    if (safeCurrentViewIndex().row() == -1)
        return -1;
    else
        return m_issuesModel->item(m_proxyIssuesModel->mapToSource(safeCurrentViewIndex()).row(), Globals::IdColumn)->text().toInt();
}

/**
 * @brief MainWindow::currentIssueId
 *  Returns Redmine ID of the currently logged issue.
 */
int MainWindow::currentIssueId() const
{
    return m_currentIssueId;
}

QModelIndex MainWindow::currentIssueIndex() const
{
    QList<QStandardItem *> issues = m_issuesModel->findItems(QString::number(currentIssueId()), Qt::MatchExactly, Globals::IdColumn);

    if (issues.size() == 0)
        return QModelIndex();
    else
        return issues.first()->index();
}

/**
 * @brief MainWindow::safeCurrentViewIndex
 *  Returns index of the selected item in the issues view. If no items are selected, will select the first one and return it's index.
 */
QModelIndex MainWindow::safeCurrentViewIndex() const
{
    if (!ui->issuesView->currentIndex().isValid()) {
        ui->issuesView->setCurrentIndex(m_proxyIssuesModel->index(0, 0));
    }

    return ui->issuesView->currentIndex();
}
