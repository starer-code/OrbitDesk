#ifndef NOTE_MANAGER_H
#define NOTE_MANAGER_H

#include <QObject>
#include <QList>

struct Note {
    int id;
    QString title;
    QString content;
    QString tags;
    bool isPinned;
    QString createdAt;
    QString updatedAt;
};

class NoteManager : public QObject
{
    Q_OBJECT

public:
    explicit NoteManager(QObject *parent = nullptr);

    bool addNote(const QString &title, const QString &content, const QString &tags = "");
    bool updateNote(int id, const QString &title, const QString &content, const QString &tags = "");
    bool deleteNote(int id);
    bool togglePin(int id);
    Note getNote(int id);
    QList<Note> getAllNotes();
    QList<Note> searchNotes(const QString &keyword);

private:
    void initDatabase();
};

#endif // NOTE_MANAGER_H
