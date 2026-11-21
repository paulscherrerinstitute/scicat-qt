#include "session.h"

#include "clitool.h"
#include "settings.h"

using namespace scicat;

Session::Session(ApiClient *api, Settings *settings, const QString &applicationDir, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_settings(settings)
    , m_applicationDir(applicationDir)
{
    m_api->setServerId(settings->store()->value(QStringLiteral("serverID")).toInt());
}

void Session::setServerId(int id)
{
    id = Servers::validId(id);
    if (id == serverId())
        return;
    // a token belongs to one SciCat: changing the server ends the session
    if (m_state != SignedOut)
        signOut();
    m_api->setServerId(id);
    m_settings->store()->setValue(QStringLiteral("serverID"), id);
    clearProblem();
    emit serverChanged();
}

Session::Environment Session::environment() const
{
    if (m_api->hasApiOverride())
        return LocalTest;
    switch (Servers::at(serverId()).kind) {
    case Server::Kind::QA: return QA;
    case Server::Kind::Development: return Development;
    default: return Production;
    }
}

QString Session::environmentLabel() const
{
    return environment() == LocalTest ? tr("Local test") : serverName();
}

QString Session::apiHost() const
{
    const QUrl url(m_api->apiBase());
    return url.port() > 0 ? QStringLiteral("%1:%2").arg(url.host()).arg(url.port()) : url.host();
}

QString Session::tokenUrlText() const
{
    const QUrl url = tokenUrl();
    return url.host() + url.path();
}

void Session::signIn(const QString &tokenInput)
{
    if (m_state != SignedOut)
        return;
    const QString token = tokenInput.trimmed();
    if (token.isEmpty()) {
        setProblem(TokenMissing, tr("Paste your token first."));
        return;
    }
    clearProblem();
    m_api->setToken(token);
    setState(SigningIn);

    ApiReply *reply = m_api->get(m_api->request(QStringLiteral("auth/whoami"), ApiVersion::V3));
    connect(reply, &ApiReply::finished, this, [this](ApiReply *answer) {
        if (m_state != SigningIn)
            return;
        UserInfo user;
        switch (parseWhoami(answer->httpStatus(), answer->json(), &user)) {
        case AuthState::Valid:
            if (user.id.isEmpty())
                finishSignIn(user);
            else
                askForIdentityEmail(user);
            break;
        case AuthState::Invalid:
            m_api->setToken(QString());
            setState(SignedOut);
            setProblem(TokenRejected, tr("SciCat did not accept this token. Tokens expire: get a new one and paste it here."));
            break;
        case AuthState::ConnectionError:
            m_api->setToken(QString());
            setState(SignedOut);
            setProblem(NoConnection, tr("Could not reach SciCat at %1. Check that you are inside the PSI network or on VPN, then try again.").arg(apiHost()));
            break;
        }
    });
}

// As the Widgets GUI does: the address of the identity profile where there is one, else the one auth/whoami gave.
// The token is valid at this point, so a failure of this second request does not stop the sign-in.
void Session::askForIdentityEmail(const UserInfo &user)
{
    ApiReply *reply = m_api->get(m_api->request(userIdentityEndpoint(user.id), ApiVersion::V3));
    connect(reply, &ApiReply::finished, this, [this, user](ApiReply *answer) {
        if (m_state != SigningIn)
            return;
        UserInfo known = user;
        const QString identityEmail = answer->ok() ? parseIdentityEmail(answer->json()) : QString();
        if (!identityEmail.isEmpty())
            known.email = identityEmail;
        finishSignIn(known);
    });
}

void Session::finishSignIn(const UserInfo &user)
{
    m_user = user;
    // prefer the scicat-cli bundled with the GUI, then the one on PATH; keep the stored one when none is found
    const QString cli = locateCli(m_applicationDir);
    if (!cli.isEmpty())
        m_settings->setCliPath(cli);
    emit userChanged();
    setState(SignedIn);
}

void Session::signOut()
{
    if (m_state == SignedOut)
        return;
    emit aboutToSignOut();
    m_api->setToken(QString());
    m_user = UserInfo();
    emit userChanged();
    setState(SignedOut);
}

void Session::clearProblem()
{
    setProblem(NoProblem, QString());
}

void Session::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

void Session::setProblem(Problem problem, const QString &text)
{
    if (m_problem == problem && m_problemText == text)
        return;
    m_problem = problem;
    m_problemText = text;
    emit problemChanged();
}
