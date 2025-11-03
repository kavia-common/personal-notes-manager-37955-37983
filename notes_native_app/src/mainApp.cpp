#include "mainApp.h"

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QStyle>
#include <optional>
#include <QRandomGenerator>

// ========== ThemeManager ==========
ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() : QObject(nullptr) {
    // default already set in struct
}

ThemeConfig ThemeManager::theme() const {
    return m_theme;
}

void ThemeManager::setDarkMode(bool enabled) {
    if (m_theme.darkMode != enabled) {
        m_theme.darkMode = enabled;
        emit themeChanged();
    }
}

QString ThemeManager::appStyleSheet() const {
    const auto t = m_theme;
    // Minimal modern theme with rounded corners and subtle shadows via palette; Qt stylesheets don't support shadows globally.
    QString bg = t.darkMode ? "#0f172a" : t.background;
    QString surface = t.darkMode ? "#111827" : t.surface;
    QString text = t.darkMode ? "#F3F4F6" : t.text;

    return QString(
        "QWidget { background: %1; color: %2; } "
        "QLineEdit, QTextEdit, QComboBox, QListWidget { "
        "  background: %3; color: %2; border: 1px solid rgba(0,0,0,0.08); border-radius: 10px; padding: 8px; } "
        "QPushButton { background: %4; color: white; border: none; border-radius: 10px; padding: 8px 14px; } "
        "QPushButton:hover { filter: brightness(1.05); } "
        "QPushButton#secondary { background: %5; } "
        "QPushButton#danger { background: %6; } "
        "QListWidget::item { padding: 10px; border-bottom: 1px solid rgba(0,0,0,0.06); } "
        "QListWidget::item:selected { background: rgba(37,99,235,0.12); } "
        "QCheckBox { padding: 6px; } "
    ).arg(bg, text, surface, t.primary, t.secondary, t.error);
}

// ========== NotesStorage ==========
static QJsonObject noteToJson(const Note& n) {
    QJsonObject obj;
    obj["id"] = n.id;
    obj["title"] = n.title;
    obj["body"] = n.body;
    obj["tags"] = QJsonArray::fromStringList(n.tags);
    obj["createdAt"] = n.createdAt.toUTC().toString(Qt::ISODate);
    obj["updatedAt"] = n.updatedAt.toUTC().toString(Qt::ISODate);
    return obj;
}

static std::optional<Note> noteFromJson(const QJsonObject& obj) {
    Note n;
    if (!obj.contains("id")) return std::nullopt;
    n.id = obj["id"].toString();
    n.title = obj["title"].toString();
    n.body = obj["body"].toString();
    QStringList tags;
    if (obj.contains("tags") && obj["tags"].isArray()) {
        for (const auto& v : obj["tags"].toArray()) tags << v.toString();
    }
    n.tags = tags;
    n.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate).toUTC();
    n.updatedAt = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate).toUTC();
    return n;
}

NotesStorage::NotesStorage(QObject* parent) : QObject(parent) {}

QString NotesStorage::storagePath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/notes.json";
}

bool NotesStorage::seedIfEmpty() {
    if (!m_notes.isEmpty()) return false;
    // Seed demo notes
    Note n1;
    n1.id = generateId();
    n1.title = "Welcome to Notes";
    n1.body = "This is your personal notes app.\n- Create notes\n- Edit notes\n- Search notes\nEnjoy!";
    n1.tags = {"demo", "welcome"};
    n1.createdAt = QDateTime::currentDateTimeUtc();
    n1.updatedAt = n1.createdAt;

    Note n2;
    n2.id = generateId();
    n2.title = "Ocean Professional Theme";
    n2.body = "Primary: #2563EB\nSecondary: #F59E0B\nError: #EF4444\nBackground: #f9fafb\nSurface: #ffffff\nText: #111827";
    n2.tags = {"theme", "colors"};
    n2.createdAt = QDateTime::currentDateTimeUtc();
    n2.updatedAt = n2.createdAt;

    m_notes.append(n1);
    m_notes.append(n2);
    return true;
}

bool NotesStorage::load() {
    QFile f(storagePath());
    if (!f.exists()) {
        // first run; seed and save
        seedIfEmpty();
        return save();
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    const auto data = f.readAll();
    f.close();
    QJsonParseError err{};
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        // Attempt recover: backup and reseed
        QFile::rename(storagePath(), storagePath() + ".corrupt");
        m_notes.clear();
        seedIfEmpty();
        return save();
    }
    m_notes.clear();
    if (doc.isArray()) {
        for (const auto& v : doc.array()) {
            if (!v.isObject()) continue;
            auto n = noteFromJson(v.toObject());
            if (n) m_notes.append(*n);
        }
    }
    if (m_notes.isEmpty()) {
        // seed when file existed but empty content
        seedIfEmpty();
        save();
    }
    emit notesChanged();
    return true;
}

bool NotesStorage::save() const {
    QJsonArray arr;
    for (const auto& n : m_notes) arr.append(noteToJson(n));
    QJsonDocument doc(arr);
    QFile f(storagePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

QList<Note> NotesStorage::list(const QString& search, const QString& sortKey, bool ascending) const {
    QList<Note> out = m_notes;
    if (!search.trimmed().isEmpty()) {
        const auto q = search.trimmed().toLower();
        QList<Note> filtered;
        for (const auto& n : out) {
            if (n.title.toLower().contains(q) || n.body.toLower().contains(q)) {
                filtered.append(n);
            }
        }
        out = filtered;
    }
    std::sort(out.begin(), out.end(), [&](const Note& a, const Note& b) {
        if (sortKey == "title") {
            if (ascending) return a.title.toLower() < b.title.toLower();
            return a.title.toLower() > b.title.toLower();
        } else {
            // updatedAt default
            if (ascending) return a.updatedAt < b.updatedAt;
            return a.updatedAt > b.updatedAt;
        }
    });
    return out;
}

QString NotesStorage::generateId() const {
    // Combine current timestamp with a bounded random number for uniqueness.
    // QRandomGenerator is the Qt6-compliant RNG; global() returns a thread-safe generator.
    const quint32 rnd = QRandomGenerator::global()->bounded(100000u);
    return QString::number(QDateTime::currentMSecsSinceEpoch()) + "-" + QString::number(rnd);
}

Note NotesStorage::upsert(const Note& note) {
    Note out = note;
    const bool isNew = note.id.isEmpty();
    if (isNew) {
        out.id = generateId();
        out.createdAt = QDateTime::currentDateTimeUtc();
    }
    out.updatedAt = QDateTime::currentDateTimeUtc();

    bool replaced = false;
    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes[i].id == out.id) {
            m_notes[i] = out;
            replaced = true;
            break;
        }
    }
    if (!replaced) m_notes.prepend(out);
    save();
    emit notesChanged();
    return out;
}

bool NotesStorage::remove(const QString& id) {
    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes[i].id == id) {
            m_notes.removeAt(i);
            save();
            emit notesChanged();
            return true;
        }
    }
    return false;
}

std::optional<Note> NotesStorage::get(const QString& id) const {
    for (const auto& n : m_notes) if (n.id == id) return n;
    return std::nullopt;
}

// ========== NotesListWidget ==========
NotesListWidget::NotesListWidget(NotesStorage* storage, QWidget* parent)
    : QWidget(parent), m_list(new QListWidget(this)), m_search(new QLineEdit(this)),
      m_sort(new QComboBox(this)), m_addButton(new QPushButton("New Note", this)),
      m_settingsButton(new QPushButton("Settings", this)), m_storage(storage) {
    auto* root = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();
    m_search->setPlaceholderText("Search notes...");
    m_sort->addItem("Updated date (newest)", QVariant::fromValue(QString("updatedAt")));
    m_sort->addItem("Title (A-Z)", QVariant::fromValue(QString("title")));
    m_addButton->setObjectName("primary");
    m_settingsButton->setObjectName("secondary");
    m_settingsButton->setText("Settings");

    topBar->addWidget(m_search);
    topBar->addWidget(m_sort);
    topBar->addWidget(m_addButton);
    topBar->addWidget(m_settingsButton);

    root->addLayout(topBar);
    root->addWidget(m_list);

    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setUniformItemSizes(true);

    connect(m_search, &QLineEdit::textChanged, this, &NotesListWidget::onSearchChanged);
    connect(m_sort, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NotesListWidget::onSortChanged);
    connect(m_addButton, &QPushButton::clicked, this, &NotesListWidget::onAddClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &NotesListWidget::openSettingsRequested);
    connect(m_list, &QListWidget::itemActivated, this, &NotesListWidget::onItemActivated);
    connect(m_storage, &NotesStorage::notesChanged, this, &NotesListWidget::refreshList);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &NotesListWidget::applyTheme);

    applyTheme();
    refreshList();
}

void NotesListWidget::applyTheme() {
    setStyleSheet(ThemeManager::instance().appStyleSheet());
}

void NotesListWidget::refreshList() {
    m_list->clear();
    const int idx = m_sort->currentIndex();
    m_sortKey = (idx == 1) ? "title" : "updatedAt";
    m_sortAscending = (idx == 1); // title ascending, date descending
    for (const auto& n : m_storage->list(m_searchText, m_sortKey, m_sortAscending)) {
        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, n.id);
        item->setText(n.title.isEmpty() ? "(Untitled)" : n.title);
        item->setToolTip(n.body.left(200));
        m_list->addItem(item);
    }
}

void NotesListWidget::onSearchChanged(const QString& text) {
    m_searchText = text;
    refreshList();
}

void NotesListWidget::onSortChanged(int /*idx*/) {
    refreshList();
}

void NotesListWidget::onAddClicked() {
    emit createNoteRequested();
}

void NotesListWidget::onItemActivated() {
    auto* item = m_list->currentItem();
    if (!item) return;
    const auto id = item->data(Qt::UserRole).toString();
    emit editNoteRequested(id);
}

// ========== NoteEditorWidget ==========
NoteEditorWidget::NoteEditorWidget(NotesStorage* storage, QWidget* parent)
    : QWidget(parent), m_title(new QLineEdit(this)), m_body(new QTextEdit(this)),
      m_tags(new QLineEdit(this)), m_save(new QPushButton("Save", this)),
      m_delete(new QPushButton("Delete", this)), m_back(new QPushButton("Back", this)),
      m_storage(storage) {
    auto* root = new QVBoxLayout(this);

    auto* header = new QHBoxLayout();
    m_back->setObjectName("secondary");
    m_save->setObjectName("primary");
    m_delete->setObjectName("danger");
    header->addWidget(m_back);
    header->addStretch();
    header->addWidget(m_save);
    header->addWidget(m_delete);

    m_title->setPlaceholderText("Title");
    m_tags->setPlaceholderText("Tags (comma separated)");
    m_body->setPlaceholderText("Write your note here...");

    root->addLayout(header);
    root->addWidget(m_title);
    root->addWidget(m_tags, 0);
    root->addWidget(m_body, 1);

    connect(m_save, &QPushButton::clicked, this, &NoteEditorWidget::onSave);
    connect(m_delete, &QPushButton::clicked, this, &NoteEditorWidget::onDelete);
    connect(m_back, &QPushButton::clicked, this, &NoteEditorWidget::backRequested);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &NoteEditorWidget::applyTheme);

    applyTheme();
}

void NoteEditorWidget::applyTheme() {
    setStyleSheet(ThemeManager::instance().appStyleSheet());
}

void NoteEditorWidget::loadNote(const QString& id) {
    m_currentId = id;
    if (id.isEmpty()) {
        m_title->clear();
        m_body->clear();
        m_tags->clear();
        m_delete->setEnabled(false);
    } else {
        auto n = m_storage->get(id);
        if (!n) {
            QMessageBox::warning(this, "Not found", "The note could not be found.");
            return;
        }
        m_title->setText(n->title);
        m_body->setText(n->body);
        m_tags->setText(n->tags.join(", "));
        m_delete->setEnabled(true);
    }
}

void NoteEditorWidget::onSave() {
    // Build the note from UI inputs
    Note n;
    n.id = m_currentId;
    n.title = m_title->text();
    n.body = m_body->toPlainText();
    const auto tagsText = m_tags->text().trimmed();
    if (!tagsText.isEmpty()) {
        for (const auto& part : tagsText.split(',', Qt::SkipEmptyParts)) {
            n.tags << part.trimmed();
        }
    }

    // Persist via storage and receive the saved copy (with id/timestamps)
    Note savedNote = m_storage->upsert(n);
    m_currentId = savedNote.id;

    // Emit signal with the saved note payload
    emit saved(savedNote);

    QMessageBox::information(this, "Saved", "Your note has been saved.");
}

void NoteEditorWidget::onDelete() {
    if (m_currentId.isEmpty()) {
        emit backRequested();
        return;
    }
    auto res = QMessageBox::question(this, "Delete note", "Are you sure you want to delete this note?");
    if (res == QMessageBox::Yes) {
        if (m_storage->remove(m_currentId)) {
            // Emit a signal to inform list to refresh; provide a minimal note payload
            Note deletedNote;
            deletedNote.id = m_currentId;
            emit saved(deletedNote);
            emit backRequested();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete the note.");
        }
    }
}

// ========== SettingsWidget ==========
SettingsWidget::SettingsWidget(QWidget* parent)
    : QWidget(parent), m_darkToggle(new QCheckBox("Enable Dark Mode", this)), m_back(new QPushButton("Back", this)) {
    auto* root = new QVBoxLayout(this);

    m_back->setObjectName("secondary");

    auto* header = new QHBoxLayout();
    header->addWidget(m_back);
    header->addStretch();

    root->addLayout(header);
    root->addSpacing(8);
    auto* card = new QWidget(this);
    auto* cardLayout = new QVBoxLayout(card);
    auto* label = new QLabel("Appearance", card);
    QFont f = label->font();
    f.setPointSize(f.pointSize() + 1);
    f.setBold(true);
    label->setFont(f);
    cardLayout->addWidget(label);
    cardLayout->addWidget(m_darkToggle);
    card->setObjectName("surfaceCard");
    card->setStyleSheet("QWidget#surfaceCard { border: 1px solid rgba(0,0,0,0.08); border-radius: 12px; background: palette(base); padding: 12px; }");

    root->addWidget(card);
    root->addStretch();

    connect(m_back, &QPushButton::clicked, this, &SettingsWidget::backRequested);
    connect(m_darkToggle, &QCheckBox::toggled, this, &SettingsWidget::onToggleTheme);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &SettingsWidget::applyTheme);

    // Initialize
    m_darkToggle->setChecked(ThemeManager::instance().theme().darkMode);
    applyTheme();
}

void SettingsWidget::onToggleTheme(bool checked) {
    ThemeManager::instance().setDarkMode(checked);
}

void SettingsWidget::applyTheme() {
    setStyleSheet(ThemeManager::instance().appStyleSheet());
}

// ========== MainWindow ==========
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_storage(new NotesStorage(this)),
      m_listPage(new NotesListWidget(m_storage, this)),
      m_editorPage(new NoteEditorWidget(m_storage, this)),
      m_settingsPage(new SettingsWidget(this)) {

    setWindowTitle("Personal Notes");
    setCentralWidget(m_listPage);
    resize(900, 600);

    connect(m_listPage, &NotesListWidget::createNoteRequested, this, &MainWindow::openCreate);
    connect(m_listPage, &NotesListWidget::editNoteRequested, this, &MainWindow::openEdit);
    connect(m_listPage, &NotesListWidget::openSettingsRequested, this, &MainWindow::openSettings);
    connect(m_editorPage, &NoteEditorWidget::backRequested, this, &MainWindow::openList);
    connect(m_editorPage, &NoteEditorWidget::saved, m_listPage, &NotesListWidget::refreshList);
    connect(m_settingsPage, &SettingsWidget::backRequested, this, &MainWindow::openList);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::applyTheme);

    // Load storage after UI constructed
    m_storage->load();

    applyTheme();
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(ThemeManager::instance().appStyleSheet());
}

void MainWindow::openCreate() {
    setCentralWidget(m_editorPage);
    m_editorPage->loadNote("");
}

void MainWindow::openEdit(const QString& id) {
    setCentralWidget(m_editorPage);
    m_editorPage->loadNote(id);
}

void MainWindow::openSettings() {
    setCentralWidget(m_settingsPage);
}

void MainWindow::openList() {
    setCentralWidget(m_listPage);
    m_listPage->refreshList();
}

// ========== main ==========
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Apply initial theme stylesheet
    app.setStyleSheet(ThemeManager::instance().appStyleSheet());

    MainWindow win;
    win.show();

    return app.exec();
}
