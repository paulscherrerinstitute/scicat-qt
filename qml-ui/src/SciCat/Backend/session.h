#pragma once

#include "apiclient.h"
#include "catalog.h"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class Settings;

// Who is signed in to which SciCat. Token-only, as in the Widgets GUI after #17.
class Session : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.session")

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY stateChanged)
    Q_PROPERTY(int serverId READ serverId WRITE setServerId NOTIFY serverChanged)
    Q_PROPERTY(QStringList serverNames READ serverNames CONSTANT)
    Q_PROPERTY(QString serverName READ serverName NOTIFY serverChanged)
    Q_PROPERTY(Environment environment READ environment NOTIFY serverChanged)
    Q_PROPERTY(QString environmentLabel READ environmentLabel NOTIFY serverChanged)
    Q_PROPERTY(QString apiHost READ apiHost NOTIFY serverChanged)
    Q_PROPERTY(QUrl tokenUrl READ tokenUrl NOTIFY serverChanged)
    Q_PROPERTY(QString tokenUrlText READ tokenUrlText NOTIFY serverChanged)
    Q_PROPERTY(QString username READ username NOTIFY userChanged)
    Q_PROPERTY(QString email READ email NOTIFY userChanged)
    Q_PROPERTY(QStringList groups READ groups NOTIFY userChanged)
    Q_PROPERTY(Problem problem READ problem NOTIFY problemChanged)
    Q_PROPERTY(QString problemText READ problemText NOTIFY problemChanged)

public:
    enum State { SignedOut, SigningIn, SignedIn };
    Q_ENUM(State)
    enum Environment { Production, QA, Development, LocalTest };
    Q_ENUM(Environment)
    enum Problem { NoProblem, TokenMissing, TokenRejected, NoConnection };
    Q_ENUM(Problem)

    Session(scicat::ApiClient *api, Settings *settings, const QString &applicationDir, QObject *parent = nullptr);

    State state() const { return m_state; }
    bool signedIn() const { return m_state == SignedIn; }
    int serverId() const { return m_api->serverId(); }
    void setServerId(int id);
    QStringList serverNames() const { return scicat::Servers::names(); }
    QString serverName() const { return scicat::Servers::at(serverId()).name; }
    Environment environment() const;
    QString environmentLabel() const;
    QString apiHost() const;
    QUrl tokenUrl() const { return QUrl(scicat::Servers::tokenUrl(serverId())); }
    QString tokenUrlText() const;
    QString username() const { return m_user.username; }
    QString email() const { return m_user.email; }
    QStringList groups() const { return m_user.groups; }
    Problem problem() const { return m_problem; }
    QString problemText() const { return m_problemText; }
    QString token() const { return m_api->token(); }

    Q_INVOKABLE void signIn(const QString &token);
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void clearProblem();

signals:
    void stateChanged();
    void serverChanged();
    void userChanged();
    void problemChanged();
    void aboutToSignOut();

private:
    void askForIdentityEmail(const scicat::UserInfo &user);
    void finishSignIn(const scicat::UserInfo &user);
    void setState(State state);
    void setProblem(Problem problem, const QString &text);

    scicat::ApiClient *m_api;
    Settings *m_settings;
    QString m_applicationDir;
    State m_state = SignedOut;
    scicat::UserInfo m_user;
    Problem m_problem = NoProblem;
    QString m_problemText;
};
