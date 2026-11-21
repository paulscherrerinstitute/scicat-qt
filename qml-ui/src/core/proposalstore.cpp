#include "proposalstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace scicat {

QString proposalFilePath(const QString &logPath, const QString &pgroup)
{
    return QDir(logPath + QStringLiteral("/proposals/")).filePath(QStringLiteral("proposal_") + pgroup + QStringLiteral(".dat"));
}

bool loadProposalFile(const QString &logPath, const QString &pgroup, Proposal *proposal)
{
    QFile file(proposalFilePath(logPath, pgroup));
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QTextStream in(&file);
    QStringList lines;
    while (!in.atEnd())
        lines << in.readLine();
    if (lines.size() < 3)
        return false;
    *proposal = Proposal();
    proposal->piFirstName = lines.at(0);
    proposal->piLastName = lines.at(1);
    proposal->piEmail = lines.at(2);
    return true;
}

bool saveProposalFile(const QString &logPath, const QString &pgroup, const Proposal &proposal, QString *error)
{
    const QString path = proposalFilePath(logPath, pgroup);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (error)
            *error = QStringLiteral("Could not create the folder %1.").arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath()));
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Could not write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    QTextStream out(&file);
    out << proposal.piFirstName << "\n" << proposal.piLastName << "\n" << proposal.piEmail << "\n";
    return true;
}

} // namespace scicat
