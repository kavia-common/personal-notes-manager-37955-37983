#ifndef MAINAPP_H
#define MAINAPP_H

#include <QWidget>
#include <QMainWindow>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QColor>

/**
 * Ocean Professional Theme configuration tokens.
 */
struct ThemeConfig {
    QString name = "Ocean Professional";
    QString description = "Blue & amber accents";
    QString primary = "#2563EB";    // Blue
    QString secondary = "#F59E0B";  // Amber (also success)
    QString success = "#F59E0B";
    QString error = "#EF4444";
    QString background = "#f9fafb";
    QString surface = "#ffffff";
    QString text = "#111827";
    bool darkMode = false;
};

// PUBLIC_INTERFACE
class ThemeManager : public QObject {
    Q_OBJECT
public:
    /**
     * Returns the singleton ThemeManager instance.
     */
    static ThemeManager& instance();

    /**
     * Get the current theme configuration.
     */
    ThemeConfig theme() const;

    /**
     * Toggle dark mode setting.
     */
    void setDarkMode(bool enabled);

    /**
     * Build a stylesheet string applying the theme.
     */
    QString appStyleSheet() const;

signals:
    void themeChanged();

private:
    ThemeManager();
    ThemeConfig m_theme;
};

// Note model
struct Note {
    QString id;
    QString title;
    QString body;
    QStringList tags;
    QDateTime createdAt;
    QDateTime updatedAt;
};

// PUBLIC_INTERFACE
class NotesStorage : public QObject {
    Q_OBJECT
public:
    /**
     * Local JSON-based persistent storage for notes.
     * Provides CRUD operations and loads/saves to a file in writable location.
     */
    explicit NotesStorage(QObject* parent = nullptr);

    /**
     * Load notes from disk; seeds demo data on first launch when empty file.
     */
    bool load();

    /**
     * Persist current notes to disk.
     */
    bool save() const;

    /**
     * Return list of notes; optionally filtered by search text.
     */
    QList<Note> list(const QString& search = QString(), const QString& sortKey = "updatedAt", bool ascending = false) const;

    /**
     * Create or update a note. If id empty, creates a new one.
     */
    Note upsert(const Note& note);

    /**
     * Delete a note by id.
     */
    bool remove(const QString& id);

    /**
     * Get note by id.
     */
    std::optional<Note> get(const QString& id) const;

signals:
    void notesChanged();

private:
    QString storagePath() const;
    QString generateId() const;
    bool seedIfEmpty();

    QList<Note> m_notes;
};

// PUBLIC_INTERFACE
class NotesListWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * Notes list screen with search and sort controls.
     * Emits signals for creating and editing notes.
     */
    explicit NotesListWidget(NotesStorage* storage, QWidget* parent = nullptr);

signals:
    void createNoteRequested();
    void editNoteRequested(const QString& id);
    void openSettingsRequested();

public slots:
    void refreshList();

private slots:
    void onSearchChanged(const QString& text);
    void onSortChanged(int idx);
    void onAddClicked();
    void onItemActivated();

private:
    class QListWidget* m_list;
    class QLineEdit* m_search;
    class QComboBox* m_sort;
    class QPushButton* m_addButton;
    class QPushButton* m_settingsButton;
    NotesStorage* m_storage;
    QString m_searchText;
    QString m_sortKey = "updatedAt";
    bool m_sortAscending = false;

    void applyTheme();
};

// PUBLIC_INTERFACE
class NoteEditorWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * Editor screen for creating/editing a note.
     */
    explicit NoteEditorWidget(NotesStorage* storage, QWidget* parent = nullptr);

    /**
     * Load a note by id for editing; if empty id, sets up a new note draft.
     */
    void loadNote(const QString& id);

signals:
    void backRequested();
    void saved();

private slots:
    void onSave();
    void onDelete();

private:
    class QLineEdit* m_title;
    class QTextEdit* m_body;
    class QLineEdit* m_tags;
    class QPushButton* m_save;
    class QPushButton* m_delete;
    class QPushButton* m_back;

    NotesStorage* m_storage;
    QString m_currentId;

    void applyTheme();
};

// PUBLIC_INTERFACE
class SettingsWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * Simple settings screen for toggling light/dark theme.
     */
    explicit SettingsWidget(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onToggleTheme(bool checked);

private:
    class QCheckBox* m_darkToggle;
    class QPushButton* m_back;

    void applyTheme();
};

// PUBLIC_INTERFACE
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /**
     * Main window managing navigation between list, editor, and settings.
     */
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openCreate();
    void openEdit(const QString& id);
    void openSettings();
    void openList();

private:
    NotesStorage* m_storage;
    NotesListWidget* m_listPage;
    NoteEditorWidget* m_editorPage;
    SettingsWidget* m_settingsPage;

    void applyTheme();
};

#endif // MAINAPP_H