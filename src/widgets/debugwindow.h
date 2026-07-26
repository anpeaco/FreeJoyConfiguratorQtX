#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QWidget>
#include <QFile>
#include <QString>

namespace Ui {
class DebugWindow;
}
class QTimer;

class DebugWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DebugWindow(QWidget *parent = nullptr);
    ~DebugWindow();

    /* Severity / category of a log line. Drives the colour each line is
     * rendered in and the [TAG] it carries. App messages map here from the
     * QtMsgType in main.cpp's CustomMessageHandler; button events and the
     * marker get their own categories. */
    enum class LogLevel { Debug, Info, Warn, Error, Button, Marker };

    void retranslateUi();

    void logicalButtonState(int buttonNumber, bool state);
    void physicalButtonState(int buttonNumber, bool state);

    /* Toggled from Advanced Settings (the checkbox moved out of this widget).
     * Gates appendToLogFile; the persisted OtherSettings/LogEnabled is read on
     * construction so logging is correct whether or not this pane was opened. */
    void setWriteToFile(bool on);

    /* Mirror an already-formatted (timestamped) line from the flash / DFU
     * install progress dialogs into the on-disk log, honouring the same
     * OtherSettings/LogEnabled gate the app log uses. The caller supplies the
     * line; this adds the trailing newline. No-op when file logging is off. */
    void appendProgressLine(const QString &line);

    Q_INVOKABLE // for multithreading -- CustomMessageHandler in main posts here
        void printMsg(const QString &msg, int level = int(LogLevel::Info));

    /* Zero the per-button fire tallies (the "since reset" point). Called by
     * Log Clear AND by the Encoders tab's Reset so the two counts always share a
     * single zero point and can never drift apart. The tallies themselves live
     * in DeviceConfig (single source shared with the Encoders tab); this just
     * delegates the zeroing there. Does not touch the log text. */
    void resetFireCounts();

signals:
    /* Emitted when Log Clear zeroes the tallies, so the Encoders tab can zero its
     * matching per-row counters at the same instant. */
    void fireCountsCleared();

private slots:
    void on_pushButton_LogMarker_clicked();
    void on_pushButton_LogClear_clicked();

    /* Flush buffered log lines into the view in one batched insert (~10 Hz).
     * appendLine() only appends to m_viewBuffer -- the actual QTextBrowser
     * insertHtml, whose cost grows with document size, is moved OFF the per-line
     * (per-params-packet) hot path so high-rate logging can't stall the UI. */
    void flushViewBuffer();

private:
    Ui::DebugWindow *ui;

    /* Single sink for every log line: stamps, tags + colours by level, appends
     * to the combined view, and mirrors the plain text to the on-disk log. */
    void appendLine(LogLevel level, const QString &msg);
    void appendToLogFile(const QString &line);

    bool m_writeToFile;

    /* Persistent handle for the on-disk log. Kept OPEN across lines so we don't
     * pay a QFile::open()/close() per logged line. That syscall pair stalls the
     * UI thread for tens of ms per line when the log dir is on a cloud-synced
     * folder (OneDrive intercepts open/close for sync), which froze the app
     * during high-rate logging -- e.g. a fast-spun encoder flooding button-edge
     * lines. Reopened only when the date rolls over. See appendToLogFile. */
    QFile   m_logFile;
    QString m_logFileDate;

    /* Batched view rendering (see flushViewBuffer). m_viewBuffer accumulates
     * rendered HTML lines between flushes; m_viewLines bounds the view so its
     * insert cost can't grow without limit over a long session (the on-disk file
     * keeps the full history). */
    QString m_viewBuffer;
    QTimer *m_viewTimer = nullptr;
    int     m_viewLines = 0;
};

#endif // DEBUGWINDOW_H
