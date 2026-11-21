#include "metadatamodel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>

#include <functional>

struct MetadataModel::Node
{
    QString key;
    Kind kind = Text;
    QString text;
    Node *parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;
    bool expanded = true;

    bool container() const { return kind == Group || kind == List; }
    int depth() const
    {
        int level = -1;
        for (const Node *node = this; node->parent; node = node->parent)
            ++level;
        return level;
    }
};

MetadataModel::MetadataModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_root(std::make_unique<Node>())
{
    m_root->kind = Group;
}

MetadataModel::~MetadataModel() = default;

int MetadataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant MetadataModel::data(const QModelIndex &index, int role) const
{
    const Node *node = nodeAt(index.row());
    if (!node)
        return QVariant();
    switch (role) {
    case KeyRole:
        if (node->parent->kind == List) {
            int position = 0;
            for (const auto &sibling : node->parent->children) {
                if (sibling.get() == node)
                    break;
                ++position;
            }
            return QStringLiteral("[%1]").arg(position);
        }
        return node->key;
    case ValueRole: return node->text;
    case KindRole: return node->kind;
    case DepthRole: return node->depth();
    case ContainerRole: return node->container();
    case ExpandedRole: return node->expanded;
    case ChildCountRole: return int(node->children.size());
    case KeyEditableRole: return node->parent->kind != List;
    case KeyProblemRole: return keyProblem(node);
    case ValueProblemRole: return valueProblem(node);
    }
    return QVariant();
}

QHash<int, QByteArray> MetadataModel::roleNames() const
{
    return {{KeyRole, "key"}, {ValueRole, "value"}, {KindRole, "kind"}, {DepthRole, "depth"}, {ContainerRole, "container"},
            {ExpandedRole, "expanded"}, {ChildCountRole, "childCount"}, {KeyEditableRole, "keyEditable"},
            {KeyProblemRole, "keyProblem"}, {ValueProblemRole, "valueProblem"}};
}

int MetadataModel::fieldCount() const
{
    int fields = 0;
    std::vector<const Node *> stack{m_root.get()};
    while (!stack.empty()) {
        const Node *node = stack.back();
        stack.pop_back();
        for (const auto &child : node->children) {
            if (child->container())
                stack.push_back(child.get());
            else
                ++fields;
        }
    }
    return fields;
}

bool MetadataModel::hasProblems() const
{
    std::vector<const Node *> stack{m_root.get()};
    while (!stack.empty()) {
        const Node *node = stack.back();
        stack.pop_back();
        for (const auto &child : node->children) {
            if (!keyProblem(child.get()).isEmpty() || !valueProblem(child.get()).isEmpty())
                return true;
            stack.push_back(child.get());
        }
    }
    return false;
}

bool MetadataModel::empty() const
{
    return m_root->children.empty();
}

void MetadataModel::setJson(const QJsonObject &object)
{
    beginResetModel();
    m_root = fromValue(QString(), object);
    rebuild();
    endResetModel();
    emit countChanged();
    emit contentChanged();
}

QJsonObject MetadataModel::toJson() const
{
    return toValue(m_root.get()).toObject();
}

void MetadataModel::resetToTemplate()
{
    setJson(QJsonObject{{QStringLiteral("sample"), QJsonObject{{QStringLiteral("description"), QString()},
                                                                {QStringLiteral("name"), QString()},
                                                                {QStringLiteral("principalInvestigator"), QString()}}}});
}

int MetadataModel::addField(int parentRow)
{
    Node *parent = parentRow < 0 ? m_root.get() : nodeAt(parentRow);
    if (!parent || !parent->container())
        return -1;
    return insertChild(parent, std::make_unique<Node>());
}

int MetadataModel::addGroup(int parentRow)
{
    Node *parent = parentRow < 0 ? m_root.get() : nodeAt(parentRow);
    if (!parent || !parent->container())
        return -1;
    auto group = std::make_unique<Node>();
    group->kind = Group;
    return insertChild(parent, std::move(group));
}

void MetadataModel::removeAt(int row)
{
    Node *node = nodeAt(row);
    if (!node)
        return;
    const int last = row + visibleDescendants(node);
    beginRemoveRows(QModelIndex(), row, last);
    auto &siblings = node->parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it) {
        if (it->get() == node) {
            siblings.erase(it);
            break;
        }
    }
    rebuild();
    endRemoveRows();
    emit countChanged();
    changed();
}

void MetadataModel::setKey(int row, const QString &key)
{
    Node *node = nodeAt(row);
    if (!node || node->parent->kind == List || node->key == key)
        return;
    node->key = key;
    changed(); // siblings may gain or lose a "used twice" problem
}

void MetadataModel::setValue(int row, const QString &text)
{
    Node *node = nodeAt(row);
    if (!node || node->container() || node->text == text)
        return;
    node->text = text;
    changed(row, row);
}

void MetadataModel::setKind(int row, Kind kind)
{
    Node *node = nodeAt(row);
    if (!node || node->kind == kind)
        return;
    const bool wasContainer = node->container();
    const bool becomesContainer = kind == Group || kind == List;
    if (wasContainer && !becomesContainer && !node->children.empty()) {
        const int hidden = visibleDescendants(node);
        if (hidden > 0) {
            beginRemoveRows(QModelIndex(), row + 1, row + hidden);
            node->children.clear();
            rebuild();
            endRemoveRows();
            emit countChanged();
        } else {
            node->children.clear();
        }
    }
    node->kind = kind;
    if (kind == YesNo)
        node->text = node->text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 ? QStringLiteral("true") : QStringLiteral("false");
    if (becomesContainer)
        node->text.clear();
    changed();
}

void MetadataModel::toggle(int row)
{
    Node *node = nodeAt(row);
    if (!node || !node->container())
        return;
    if (node->expanded) {
        const int hidden = visibleDescendants(node);
        if (hidden > 0)
            beginRemoveRows(QModelIndex(), row + 1, row + hidden);
        node->expanded = false;
        rebuild();
        if (hidden > 0) {
            endRemoveRows();
            emit countChanged();
        }
    } else {
        node->expanded = true;
        const int shown = visibleDescendants(node);
        if (shown > 0)
            beginInsertRows(QModelIndex(), row + 1, row + shown);
        rebuild();
        if (shown > 0) {
            endInsertRows();
            emit countChanged();
        }
    }
    emit dataChanged(index(row), index(row), {ExpandedRole});
}

int MetadataModel::revealFirstProblem()
{
    // in the order the list shows the entries
    const std::function<Node *(const Node *)> search = [&](const Node *parent) -> Node * {
        for (const auto &child : parent->children) {
            if (!keyProblem(child.get()).isEmpty() || !valueProblem(child.get()).isEmpty())
                return child.get();
            if (Node *inside = search(child.get()))
                return inside;
        }
        return nullptr;
    };
    const Node *node = search(m_root.get());
    if (!node)
        return -1;
    std::vector<Node *> closed;
    for (Node *group = node->parent; group != m_root.get(); group = group->parent) {
        if (!group->expanded)
            closed.push_back(group);
    }
    // the outermost first: a group inside a closed group has no row yet
    for (auto it = closed.rbegin(); it != closed.rend(); ++it)
        toggle(rowOf(*it));
    return rowOf(node);
}

void MetadataModel::clear()
{
    setJson(QJsonObject());
}

QString MetadataModel::importFrom(const QUrl &fileUrl)
{
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        return tr("Could not open %1: %2").arg(file.fileName(), file.errorString());
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return tr("%1 is not valid JSON: %2").arg(file.fileName(), parseError.errorString());
    if (!document.isObject())
        return tr("%1 does not contain a JSON object.").arg(file.fileName());

    // accepted: the plain metadata object, {"scientificMetadata": {...}}, and the Widgets GUI's export
    // {"scientificMetadata": [{...}]}
    QJsonObject metadata = document.object();
    const QJsonValue wrapped = metadata.value(QLatin1String("scientificMetadata"));
    if (metadata.size() == 1 && wrapped.isObject()) {
        metadata = wrapped.toObject();
    } else if (metadata.size() == 1 && wrapped.isArray()) {
        metadata = QJsonObject();
        const QJsonArray parts = wrapped.toArray();
        for (const QJsonValue &part : parts) {
            const QJsonObject object = part.toObject();
            for (auto it = object.constBegin(); it != object.constEnd(); ++it)
                metadata.insert(it.key(), it.value());
        }
    }
    beginResetModel();
    m_beforeImport = std::move(m_root);
    m_root = fromValue(QString(), metadata);
    rebuild();
    endResetModel();
    emit countChanged();
    emit contentChanged();
    return QString();
}

void MetadataModel::undoImport()
{
    if (!m_beforeImport)
        return;
    beginResetModel();
    m_root = std::move(m_beforeImport);
    rebuild();
    endResetModel();
    emit countChanged();
    emit contentChanged();
}

QString MetadataModel::exportTo(const QUrl &fileUrl) const
{
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::WriteOnly))
        return tr("Could not write %1: %2").arg(file.fileName(), file.errorString());
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("scientificMetadata"), toJson()}}).toJson());
    return QString();
}

MetadataModel::Node *MetadataModel::nodeAt(int row) const
{
    return row >= 0 && row < int(m_rows.size()) ? m_rows[size_t(row)] : nullptr;
}

int MetadataModel::rowOf(const Node *node) const
{
    for (size_t row = 0; row < m_rows.size(); ++row) {
        if (m_rows[row] == node)
            return int(row);
    }
    return -1;
}

int MetadataModel::visibleDescendants(const Node *node) const
{
    if (!node->expanded)
        return 0;
    int visible = 0;
    for (const auto &child : node->children)
        visible += 1 + visibleDescendants(child.get());
    return visible;
}

void MetadataModel::rebuild()
{
    m_rows.clear();
    for (const auto &child : m_root->children)
        appendVisible(child.get());
}

void MetadataModel::appendVisible(Node *node)
{
    m_rows.push_back(node);
    if (!node->expanded)
        return;
    for (const auto &child : node->children)
        appendVisible(child.get());
}

int MetadataModel::insertChild(Node *parent, std::unique_ptr<Node> child)
{
    if (parent != m_root.get() && !parent->expanded)
        toggle(rowOf(parent));
    const int row = parent == m_root.get() ? int(m_rows.size()) : rowOf(parent) + visibleDescendants(parent) + 1;
    beginInsertRows(QModelIndex(), row, row);
    child->parent = parent;
    parent->children.push_back(std::move(child));
    rebuild();
    endInsertRows();
    emit countChanged();
    changed();
    return row;
}

QString MetadataModel::keyProblem(const Node *node) const
{
    if (node->parent->kind == List)
        return QString();
    if (node->key.trimmed().isEmpty())
        return tr("Give this entry a name.");
    for (const auto &sibling : node->parent->children) {
        if (sibling.get() != node && sibling->key == node->key)
            return tr("This name is used twice in this group.");
    }
    return QString();
}

QString MetadataModel::valueProblem(const Node *node) const
{
    if (node->kind != Number)
        return QString();
    bool ok = false;
    QLocale::c().toDouble(node->text.trimmed(), &ok);
    return ok ? QString() : tr("Enter a number, for example 12.4 or 1e-3.");
}

void MetadataModel::changed(int firstRow, int lastRow)
{
    if (!m_rows.empty()) {
        if (firstRow < 0)
            emit dataChanged(index(0), index(int(m_rows.size()) - 1));
        else
            emit dataChanged(index(firstRow), index(lastRow));
    }
    emit contentChanged();
}

std::unique_ptr<MetadataModel::Node> MetadataModel::fromValue(const QString &key, const QJsonValue &value)
{
    auto node = std::make_unique<Node>();
    node->key = key;
    if (value.isObject()) {
        node->kind = Group;
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            auto child = fromValue(it.key(), it.value());
            child->parent = node.get();
            node->children.push_back(std::move(child));
        }
    } else if (value.isArray()) {
        node->kind = List;
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            auto child = fromValue(QString(), item);
            child->parent = node.get();
            node->children.push_back(std::move(child));
        }
    } else if (value.isDouble()) {
        node->kind = Number;
        node->text = QLocale::c().toString(value.toDouble(), 'g', 15);
    } else if (value.isBool()) {
        node->kind = YesNo;
        node->text = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    } else {
        node->kind = Text;
        node->text = value.toString();
    }
    return node;
}

QJsonValue MetadataModel::toValue(const Node *node)
{
    switch (node->kind) {
    case Group: {
        QJsonObject object;
        for (const auto &child : node->children)
            object.insert(child->key, toValue(child.get()));
        return object;
    }
    case List: {
        QJsonArray array;
        for (const auto &child : node->children)
            array.append(toValue(child.get()));
        return array;
    }
    case Number: return QLocale::c().toDouble(node->text.trimmed());
    case YesNo: return node->text == QLatin1String("true");
    case Text: break;
    }
    return node->text;
}
