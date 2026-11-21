#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

// Scientific metadata as an editable tree, shown as an indented list.
// Values keep their JSON type: text, number, yes/no, group (object) and list (array).
class MetadataModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by the backend")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int fieldCount READ fieldCount NOTIFY contentChanged)
    Q_PROPERTY(bool hasProblems READ hasProblems NOTIFY contentChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY contentChanged)

public:
    enum Kind { Text, Number, YesNo, Group, List };
    Q_ENUM(Kind)
    enum Roles { KeyRole = Qt::UserRole + 1, ValueRole, KindRole, DepthRole, ContainerRole, ExpandedRole, ChildCountRole,
                 KeyEditableRole, KeyProblemRole, ValueProblemRole };

    explicit MetadataModel(QObject *parent = nullptr);
    ~MetadataModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_rows.size()); }
    int fieldCount() const;
    bool hasProblems() const;
    bool empty() const;

    void setJson(const QJsonObject &object);
    QJsonObject toJson() const;
    // the three sample fields the Widgets GUI starts with
    void resetToTemplate();

    // parentRow -1 adds at the top level; the new row is returned so the view can put the cursor there
    Q_INVOKABLE int addField(int parentRow);
    Q_INVOKABLE int addGroup(int parentRow);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void setKey(int row, const QString &key);
    Q_INVOKABLE void setValue(int row, const QString &text);
    Q_INVOKABLE void setKind(int row, MetadataModel::Kind kind);
    Q_INVOKABLE void toggle(int row);
    // the row of the first entry that has a problem, after opening the groups that hide it; -1 when there is none
    Q_INVOKABLE int revealFirstProblem();
    Q_INVOKABLE void clear();
    // both return an empty string on success, else what went wrong
    Q_INVOKABLE QString importFrom(const QUrl &file);
    Q_INVOKABLE QString exportTo(const QUrl &file) const;
    // an import replaces every entry; this brings back what was there before the last one, as it was
    Q_INVOKABLE void undoImport();

signals:
    void countChanged();
    void contentChanged();

private:
    struct Node;
    Node *nodeAt(int row) const;
    int rowOf(const Node *node) const;
    int visibleDescendants(const Node *node) const;
    void rebuild();
    void appendVisible(Node *node);
    int insertChild(Node *parent, std::unique_ptr<Node> child);
    QString keyProblem(const Node *node) const;
    QString valueProblem(const Node *node) const;
    void changed(int firstRow = -1, int lastRow = -1);
    static std::unique_ptr<Node> fromValue(const QString &key, const QJsonValue &value);
    static QJsonValue toValue(const Node *node);

    std::unique_ptr<Node> m_root;
    std::unique_ptr<Node> m_beforeImport;
    std::vector<Node *> m_rows;
};
