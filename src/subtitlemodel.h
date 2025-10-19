#pragma once
#include <QAbstractTableModel>
#include <QString>
#include <QVector>

struct SubtitleEntry {
    int lineNumber = 0;
    QString startTime;
    QString endTime;
    double durationSeconds = 0.0;
    int cps = 0;
    QString text;
};

class SubtitleModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SubtitleModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool loadSrt(const QString &filePath);
    void clear();

    void setTextAt(int row, const QString &text);
    bool saveSrt(const QString &filePath) const;

    enum Column {
        LineNumber = 0,
        StartTime,
        EndTime,
        CPS,
        Text,
        ColumnCount
    };

private:
    QVector<SubtitleEntry> entries_;
    static double parseTimeToSeconds(const QString &timeStr);
    static int computeCPS(const SubtitleEntry &e);

    friend class MainWindow; // optional: main window can access entries_ if needed
};
