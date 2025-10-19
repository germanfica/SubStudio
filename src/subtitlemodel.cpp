#include "subtitlemodel.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QtGlobal>
#include <cmath>

SubtitleModel::SubtitleModel(QObject *parent) : QAbstractTableModel(parent) {}

int SubtitleModel::rowCount(const QModelIndex & /*parent*/) const {
    return entries_.size();
}

int SubtitleModel::columnCount(const QModelIndex & /*parent*/) const {
    return ColumnCount;
}

QVariant SubtitleModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};

    // Always center CPS column
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == CPS) return QVariant(Qt::AlignCenter);
        return QVariant();
    }

    if (role != Qt::DisplayRole) return {};
    const SubtitleEntry &e = entries_.at(index.row());
    switch (index.column()) {
        case LineNumber: return e.lineNumber;
        case StartTime: return e.startTime;
        case EndTime: return e.endTime;
        case CPS: return e.cps;
        case Text: return e.text;
        default: return {};
    }
}

QVariant SubtitleModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case LineNumber: return QStringLiteral("#");
            case StartTime: return QStringLiteral("Start Time");
            case EndTime: return QStringLiteral("End Time");
            case CPS: return QStringLiteral("CPS");
            case Text: return QStringLiteral("Text");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void SubtitleModel::clear() {
    beginResetModel();
    entries_.clear();
    endResetModel();
}

bool SubtitleModel::loadSrt(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&f);

    QVector<SubtitleEntry> tmp;
    QString line;
    SubtitleEntry cur;
    enum State { ExpectIndex, ExpectTime, ExpectText } state = ExpectIndex;
    int indexCounter = 0;
    bool bomChecked = false;

    while (!in.atEnd()) {
        line = in.readLine();

        if (!bomChecked) {
            bomChecked = true;
            if (!line.isEmpty() && line.at(0) == QChar(0xFEFF)) {
                line.remove(0, 1);
            }
        }

        if (line.trimmed().isEmpty()) {
            if (state == ExpectText) {
                cur.cps = computeCPS(cur);
                tmp.append(cur);
                cur = SubtitleEntry();
                state = ExpectIndex;
            } else {
                state = ExpectIndex;
            }
            continue;
        }

        if (state == ExpectIndex) {
            bool ok;
            int num = line.trimmed().toInt(&ok);
            if (ok) {
                cur.lineNumber = num;
            } else {
                cur.lineNumber = ++indexCounter;
            }
            state = ExpectTime;
        } else if (state == ExpectTime) {
            QRegularExpression re(R"((\d{2}:\d{2}:\d{2}[,\.]\d{1,3})\s*-->\s*(\d{2}:\d{2}:\d{2}[,\.]\d{1,3}))");
            auto m = re.match(line);
            if (m.hasMatch()) {
                cur.startTime = m.captured(1);
                cur.endTime = m.captured(2);
                double s1 = parseTimeToSeconds(cur.startTime);
                double s2 = parseTimeToSeconds(cur.endTime);
                cur.durationSeconds = qMax(0.0, s2 - s1);
            }
            state = ExpectText;
        } else if (state == ExpectText) {
            if (!cur.text.isEmpty()) cur.text += "\n";
            cur.text += line;
        }
    }

    if (state == ExpectText && !cur.text.isEmpty()) {
        cur.cps = computeCPS(cur);
        tmp.append(cur);
    }

    beginResetModel();
    entries_ = std::move(tmp);
    endResetModel();
    return true;
}

double SubtitleModel::parseTimeToSeconds(const QString &timeStr) {
    QString s = timeStr;
    s.replace(',', '.');
    QStringList parts = s.split(':');
    if (parts.size() != 3) return 0.0;
    double hh = parts[0].toDouble();
    double mm = parts[1].toDouble();
    double ss = parts[2].toDouble();
    return hh * 3600.0 + mm * 60.0 + ss;
}

int SubtitleModel::computeCPS(const SubtitleEntry &e) {
    int chars = e.text.length();
    double dur = e.durationSeconds;
    if (dur <= 0.001) return 0;
    double cps = chars / dur;
    return static_cast<int>(std::round(cps));
}

void SubtitleModel::setTextAt(int row, const QString &text) {
    if (row < 0 || row >= entries_.size()) return;
    entries_[row].text = text;
    entries_[row].cps = computeCPS(entries_[row]);
    const QModelIndex top = index(row, Text);
    const QModelIndex bottom = index(row, Text);
    emit dataChanged(top, bottom, { Qt::DisplayRole });
    // also update CPS column
    emit dataChanged(index(row, CPS), index(row, CPS), { Qt::DisplayRole });
}

bool SubtitleModel::saveSrt(const QString &filePath) const {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream out(&f);
    out.setGenerateByteOrderMark(false);
    // write each entry in SRT format
    for (int i = 0; i < entries_.size(); ++i) {
        const SubtitleEntry &e = entries_.at(i);
        // write index (use original lineNumber when present; otherwise i+1)
        const int idx = (e.lineNumber > 0) ? e.lineNumber : (i + 1);
        out << idx << '\n';
        out << e.startTime << " --> " << e.endTime << '\n';

        // write text (already may contain newlines)
        const QStringList lines = e.text.split('\n');
        for (const QString &ln : lines) {
            out << ln << '\n';
        }

        // blank separator
        out << '\n';
    }
    f.close();
    return true;
}
