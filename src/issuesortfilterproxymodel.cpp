#include <QModelIndex>

#include "globals.h"
#include "issuesortfilterproxymodel.h"

void IssueSortFilterProxyModel::setFilterProject(const QString &project)
{
    emit(layoutAboutToBeChanged());
    m_project = project;
    invalidateFilter();
    emit(layoutChanged());
}

void IssueSortFilterProxyModel::setFilterPriority(const QString &priority)
{
    emit(layoutAboutToBeChanged());
    m_priority = priority;
    invalidateFilter();
    emit(layoutChanged());
}

void IssueSortFilterProxyModel::setFilterId(const QString &id)
{
    emit(layoutAboutToBeChanged());
    m_id = id;
    invalidateFilter();
    emit(layoutChanged());
}

void IssueSortFilterProxyModel::setFilterSubject(const QString &subject)
{
    emit(layoutAboutToBeChanged());
    m_subject = subject;
    invalidateFilter();
    emit(layoutChanged());
}

bool IssueSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // project - id - priority - subject
    QModelIndex index_project = sourceModel()->index(sourceRow, Globals::ProjectColumn, sourceParent);
    QModelIndex index_id = sourceModel()->index(sourceRow, Globals::IdColumn, sourceParent);
    QModelIndex index_priority = sourceModel()->index(sourceRow, Globals::PriorityColumn, sourceParent);
    QModelIndex index_subject = sourceModel()->index(sourceRow, Globals::SubjectColumn, sourceParent);

    return ((sourceModel()->data(index_project).toString() == m_project || m_project.isEmpty())
            && (sourceModel()->data(index_id).toString().contains(QRegExp(QString("^%1\\d*$").arg(m_id))) || m_id.isEmpty())
            && (sourceModel()->data(index_priority).toString() == m_priority || m_priority.isEmpty())
            && (sourceModel()->data(index_subject).toString().contains(m_subject, Qt::CaseInsensitive) || m_subject.isEmpty()));
}
