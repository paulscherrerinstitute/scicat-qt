#pragma once

#include "catalog.h"

// PI details typed in by hand, kept per pgroup for the next session.
// Same file and format as Experiment::savePIInfo() / getPgroupInfo() of the Widgets GUI.
namespace scicat {

QString proposalFilePath(const QString &logPath, const QString &pgroup);
bool loadProposalFile(const QString &logPath, const QString &pgroup, Proposal *proposal);
bool saveProposalFile(const QString &logPath, const QString &pgroup, const Proposal &proposal, QString *error = nullptr);

} // namespace scicat
