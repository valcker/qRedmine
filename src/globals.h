#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtCore>

namespace Globals {
    enum IssueItemRoles {
        SortRole = Qt::UserRole
    };

    enum IssueViewColumns {
        ProjectColumn = 0,
        IdColumn = 1,
        PriorityColumn = 2,
        SubjectColumn = 3
    };

    static int const RedmineDefaultActivityId = 9;
}

#endif // GLOBALS_H
