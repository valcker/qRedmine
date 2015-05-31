#ifndef ISSUESMODEL_H
#define ISSUESMODEL_H

#include <QStandardItemModel>

class IssuesModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit IssuesModel(QObject *parent = 0) : QStandardItemModel(parent) {}
    IssuesModel(int rows, int columns, QObject *parent = 0) : QStandardItemModel(rows, columns, parent) {}

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
};

#endif // ISSUESMODEL_H
