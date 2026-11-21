#pragma once

#include "apiclient.h"
#include "catalog.h"

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Session;
class Settings;

// The pgroup being worked on and its proposal: what the first page of the Widgets GUI collects.
class Workspace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.workspace")

    Q_PROPERTY(QString pgroup READ pgroup NOTIFY pgroupChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(Source source READ source NOTIFY proposalChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(QString piFirstName READ piFirstName WRITE setPiFirstName NOTIFY piChanged)
    Q_PROPERTY(QString piLastName READ piLastName WRITE setPiLastName NOTIFY piChanged)
    Q_PROPERTY(QString piEmail READ piEmail WRITE setPiEmail NOTIFY piChanged)
    Q_PROPERTY(bool piEditable READ piEditable NOTIFY proposalChanged)
    Q_PROPERTY(bool piComplete READ piComplete NOTIFY piChanged)
    Q_PROPERTY(bool piEmailValid READ piEmailValid NOTIFY piChanged)
    Q_PROPERTY(QString owner READ owner NOTIFY piChanged)
    Q_PROPERTY(QString proposalId READ proposalId NOTIFY proposalChanged)
    Q_PROPERTY(QString proposalTitle READ proposalTitle NOTIFY proposalChanged)
    Q_PROPERTY(QString proposalAbstract READ proposalAbstract NOTIFY proposalChanged)
    Q_PROPERTY(QString instrument READ instrument NOTIFY proposalChanged)
    Q_PROPERTY(bool groupReady READ groupReady NOTIFY stateChanged)
    Q_PROPERTY(bool archiveReady READ archiveReady NOTIFY readinessChanged)
    Q_PROPERTY(QStringList recentGroups READ recentGroups NOTIFY recentGroupsChanged)

public:
    enum State { NoGroup, Loading, Ready, Failed };
    Q_ENUM(State)
    enum Source { FromCatalog, FromSavedFile, Manual };
    Q_ENUM(Source)

    Workspace(scicat::ApiClient *api, Session *session, Settings *settings, QObject *parent = nullptr);

    QString pgroup() const { return m_pgroup; }
    State state() const { return m_state; }
    Source source() const { return m_source; }
    QString errorText() const { return m_errorText; }
    QString piFirstName() const { return m_proposal.piFirstName; }
    void setPiFirstName(const QString &name);
    QString piLastName() const { return m_proposal.piLastName; }
    void setPiLastName(const QString &name);
    QString piEmail() const { return m_proposal.piEmail; }
    void setPiEmail(const QString &email);
    // the PI comes from the catalogue; only when SciCat has no proposal, or one without PI details, is it typed in
    bool piEditable() const { return m_source != FromCatalog || m_catalogPiIncomplete; }
    bool piComplete() const;
    bool piEmailValid() const;
    QString defaultOwnerEmail() const;
    QString defaultPrincipalInvestigator() const;
    QString owner() const { return m_proposal.piFirstName + QLatin1Char(' ') + m_proposal.piLastName; }
    QString proposalId() const { return m_proposal.proposalId; }
    QString proposalTitle() const { return m_proposal.title; }
    QString proposalAbstract() const { return m_proposal.abstract; }
    QString instrument() const { return m_proposal.instrument; }
    bool groupReady() const { return m_state == Ready; }
    bool archiveReady() const { return groupReady() && piComplete(); }
    QStringList recentGroups() const;

    // empty when the group can be used, else what is wrong with it
    Q_INVOKABLE QString problemWith(const QString &group) const;
    Q_INVOKABLE bool select(const QString &group);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void clear();
    // keeps hand-entered PI details for next time (when that preference is on); returns a warning or nothing
    Q_INVOKABLE QString rememberPi();

signals:
    void pgroupChanged();
    void stateChanged();
    void proposalChanged();
    void piChanged();
    void readinessChanged();
    void recentGroupsChanged();

private:
    void setState(State state, const QString &errorText = QString());
    void load();
    void applyProposal(const scicat::Proposal &proposal, Source source);

    scicat::ApiClient *m_api;
    Session *m_session;
    Settings *m_settings;
    QString m_pgroup;
    State m_state = NoGroup;
    Source m_source = Manual;
    QString m_errorText;
    scicat::Proposal m_proposal;
    QPointer<scicat::ApiReply> m_pending;
    bool m_piEdited = false;
    bool m_catalogPiIncomplete = false;
};
