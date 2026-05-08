#include "note_manager.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

NoteManager::NoteManager(QObject *parent)
    : QObject(parent)
{
}

bool NoteManager::addNote(const QString &title, const QString &content, const QString &tags)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO notes (title, content, tags, created_at, updated_at) "
        "VALUES (?, ?, ?, datetime('now'), datetime('now'))"
    );
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(tags);

    if (!query.exec()) {
        qDebug() << "Add note failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool NoteManager::updateNote(int id, const QString &title, const QString &content, const QString &tags)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "UPDATE notes SET title = ?, content = ?, tags = ?, updated_at = datetime('now') "
        "WHERE id = ?"
    );
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(tags);
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Update note failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool NoteManager::deleteNote(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Delete note failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool NoteManager::togglePin(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE notes SET is_pinned = CASE WHEN is_pinned = 1 THEN 0 ELSE 1 END WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Toggle pin failed:" << query.lastError().text();
        return false;
    }
    return true;
}

Note NoteManager::getNote(int id)
{
    Note note;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return note;

    QSqlQuery query(db);
    query.prepare("SELECT id, title, content, tags, is_pinned, created_at, updated_at FROM notes WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.content = query.value(2).toString();
        note.tags = query.value(3).toString();
        note.isPinned = query.value(4).toBool();
        note.createdAt = query.value(5).toString();
        note.updatedAt = query.value(6).toString();
    }
    return note;
}

QList<Note> NoteManager::getAllNotes()
{
    QList<Note> notes;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return notes;

    QSqlQuery query(db);
    query.exec("SELECT id, title, content, tags, is_pinned, created_at, updated_at FROM notes ORDER BY is_pinned DESC, updated_at DESC");

    while (query.next()) {
        Note note;
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.content = query.value(2).toString();
        note.tags = query.value(3).toString();
        note.isPinned = query.value(4).toBool();
        note.createdAt = query.value(5).toString();
        note.updatedAt = query.value(6).toString();
        notes.append(note);
    }
    return notes;
}

QList<Note> NoteManager::searchNotes(const QString &keyword)
{
    QList<Note> notes;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return notes;

    QSqlQuery query(db);
    query.prepare(
        "SELECT id, title, content, tags, is_pinned, created_at, updated_at "
        "FROM notes WHERE title LIKE ? OR content LIKE ? OR tags LIKE ? "
        "ORDER BY is_pinned DESC, updated_at DESC"
    );
    QString pattern = "%" + keyword + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);

    if (query.exec()) {
        while (query.next()) {
            Note note;
            note.id = query.value(0).toInt();
            note.title = query.value(1).toString();
            note.content = query.value(2).toString();
            note.tags = query.value(3).toString();
            note.isPinned = query.value(4).toBool();
            note.createdAt = query.value(5).toString();
            note.updatedAt = query.value(6).toString();
            notes.append(note);
        }
    }
    return notes;
}
