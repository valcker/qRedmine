#ifndef ISSUESORTFILTERPROXYMODEL_H
#define ISSUESORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QString>

class IssueSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit IssueSortFilterProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {}

    QString filterProject() const { return m_project; }
    void setFilterProject(const QString &project);
    QString filterPriority() const { return m_priority; }
    void setFilterPriority(const QString &priority);
    QString filterId() const { return m_id; }
    void setFilterId(const QString &id);
    QString filterSubject() const { return m_subject; }
    void setFilterSubject(const QString &subject);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QString m_project;
    QString m_priority;
    QString m_id;
    QString m_subject;
};

#endif // ISSUESORTFILTERPROXYMODEL_H
