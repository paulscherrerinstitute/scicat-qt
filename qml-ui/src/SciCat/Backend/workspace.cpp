#include "workspace.h"

#include "proposalstore.h"
#include "session.h"
#include "settings.h"

using namespace scicat;

Workspace::Workspace(ApiClient *api, Session *session, Settings *settings, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_session(session)
    , m_settings(settings)
{
    connect(this, &Workspace::stateChanged, this, &Workspace::readinessChanged);
    connect(this, &Workspace::piChanged, this, &Workspace::readinessChanged);
    connect(session, &Session::aboutToSignOut, this, &Workspace::clear);
    connect(session, &Session::stateChanged, this, [this] {
        if (!m_session->signedIn())
            return;
        emit recentGroupsChanged();
        // come back to the group used last, if the user still belongs to it
        const QString last = m_settings->ui(QStringLiteral("lastPgroup")).toString();
        if (m_pgroup.isEmpty() && m_session->groups().contains(last))
            select(last);
    });
}

void Workspace::setPiFirstName(const QString &name)
{
    if (!piEditable() || m_proposal.piFirstName == name)
        return;
    m_proposal.piFirstName = name;
    m_piEdited = true;
    emit piChanged();
}

void Workspace::setPiLastName(const QString &name)
{
    if (!piEditable() || m_proposal.piLastName == name)
        return;
    m_proposal.piLastName = name;
    m_piEdited = true;
    emit piChanged();
}

void Workspace::setPiEmail(const QString &email)
{
    if (!piEditable() || m_proposal.piEmail == email)
        return;
    m_proposal.piEmail = email;
    m_piEdited = true;
    emit piChanged();
}

// As the Widgets GUI checks it: both names, and an email. What SciCat's proposal says is taken as it is, because the
// backend may hand out a masked address; what was typed here has to be an address.
bool Workspace::piComplete() const
{
    return !m_proposal.piFirstName.trimmed().isEmpty() && !m_proposal.piLastName.trimmed().isEmpty() && piEmailValid();
}

bool Workspace::piEmailValid() const
{
    if (m_source == FromCatalog && !m_piEdited)
        return !m_proposal.piEmail.isEmpty();
    return isEmailAddress(m_proposal.piEmail.trimmed());
}

// The address a dataset gets as owner email unless another one is typed: the PI's when it is an address, else the
// user's own (a masked address of the proposal is of no use to scicat-cli).
QString Workspace::defaultOwnerEmail() const
{
    return isEmailAddress(m_proposal.piEmail) ? m_proposal.piEmail : m_session->email();
}

// The principal investigator of raw data unless another one is typed: the PI's address, else the PI's name.
QString Workspace::defaultPrincipalInvestigator() const
{
    return isEmailAddress(m_proposal.piEmail) ? m_proposal.piEmail : owner().trimmed();
}

QStringList Workspace::recentGroups() const
{
    QStringList recent;
    const QStringList stored = m_settings->ui(QStringLiteral("recentPgroups")).toStringList();
    for (const QString &group : stored) {
        if (m_session->groups().contains(group))
            recent << group;
    }
    return recent;
}

QString Workspace::problemWith(const QString &group) const
{
    switch (checkPgroup(group.trimmed(), m_session->groups())) {
    case PgroupProblem::Empty:
        return tr("Enter a pgroup.");
    case PgroupProblem::Malformed:
        return tr("A pgroup looks like p12345: p or a, then five or six characters.");
    case PgroupProblem::NotMember:
        return tr("You are not a member of %1, so SciCat will not let you use it.").arg(group.trimmed());
    case PgroupProblem::None:
        break;
    }
    return QString();
}

bool Workspace::select(const QString &groupInput)
{
    const QString group = groupInput.trimmed();
    if (!problemWith(group).isEmpty())
        return false;
    if (group == m_pgroup && m_state != Failed)
        return true;
    m_pgroup = group;
    emit pgroupChanged();

    m_settings->setUi(QStringLiteral("lastPgroup"), group);
    QStringList recent = m_settings->ui(QStringLiteral("recentPgroups")).toStringList();
    recent.removeAll(group);
    recent.prepend(group);
    m_settings->setUi(QStringLiteral("recentPgroups"), QStringList(recent.mid(0, 5)));
    emit recentGroupsChanged();

    load();
    return true;
}

void Workspace::reload()
{
    if (!m_pgroup.isEmpty())
        load();
}

void Workspace::clear()
{
    if (m_pending) {
        disconnect(m_pending, nullptr, this, nullptr);
        m_pending->abort();
    }
    m_pgroup.clear();
    m_proposal = Proposal();
    m_source = Manual;
    m_piEdited = false;
    emit pgroupChanged();
    emit proposalChanged();
    emit piChanged();
    setState(NoGroup);
}

QString Workspace::rememberPi()
{
    if (m_source == FromCatalog || !m_piEdited || !piComplete() || !m_settings->saveProposalInfo())
        return QString();
    QString error;
    if (!saveProposalFile(m_settings->logPath(), m_pgroup, m_proposal, &error))
        return tr("The PI details could not be saved for next time. %1").arg(error);
    m_piEdited = false;
    return QString();
}

void Workspace::setState(State state, const QString &errorText)
{
    if (m_state == state && m_errorText == errorText)
        return;
    m_state = state;
    m_errorText = errorText;
    emit stateChanged();
}

void Workspace::load()
{
    if (m_pending) {
        disconnect(m_pending, nullptr, this, nullptr);
        m_pending->abort();
    }
    setState(Loading);
    const QString group = m_pgroup;
    m_pending = m_api->get(m_api->request(QStringLiteral("proposals"), ApiVersion::V3, proposalFilter(group), QStringLiteral("filters")));
    connect(m_pending, &ApiReply::finished, this, [this, group](ApiReply *answer) {
        if (group != m_pgroup)
            return;
        if (!answer->ok()) {
            applyProposal(Proposal(), Manual);
            setState(Failed, answer->connectionFailed()
                                 ? tr("Could not reach SciCat to look up the proposal of %1.").arg(group)
                                 : tr("SciCat could not look up the proposal of %1 (HTTP %2): %3").arg(group).arg(answer->httpStatus()).arg(answer->serverMessage()));
            return;
        }
        Proposal proposal;
        if (parseProposals(answer->json(), &proposal))
            applyProposal(proposal, FromCatalog);
        else if (m_settings->loadProposalInfo() && loadProposalFile(m_settings->logPath(), group, &proposal))
            applyProposal(proposal, FromSavedFile);
        else
            applyProposal(Proposal(), Manual);
        setState(Ready);
    });
}

void Workspace::applyProposal(const Proposal &proposal, Source source)
{
    m_proposal = proposal;
    m_source = source;
    m_piEdited = false;
    m_catalogPiIncomplete = source == FromCatalog && !piComplete();
    emit proposalChanged();
    emit piChanged();
}
