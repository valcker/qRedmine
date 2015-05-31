#include "globals.h"
#include "issuesmodel.h"

#include <QStandardItemModel>
#include <QDebug>

QVariant IssuesModel::data(const QModelIndex &index, int role) const
{
    if ((index.column() == Globals::IdColumn || index.column() == Globals::PriorityColumn) && role == Qt::TextAlignmentRole)
        return Qt::AlignHCenter;
    else
        return QStandardItemModel::data(index, role);
}
