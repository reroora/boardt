#ifndef LOGGER_H
#define LOGGER_H

#include <QPointer>
#include <QTextEdit>

class Logger {

public:
    static Logger& getInstance() {
        static Logger instance;  // This is Meyer's singleton, more research is needed on this issue for multithreading.
        return instance;
    }

    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    void setTextEdit(QPointer<QTextEdit> textEdit);

    void log(const QString message);

private:
    Logger();

    QPointer<QTextEdit> m_textEdit = nullptr;
};

#endif // LOGGER_H;
