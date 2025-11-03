#include "logger.h"

Logger::Logger() {}

void Logger::log(const QString message) {
    if(m_textEdit) {
        m_textEdit->append(message);
    }
}

void Logger::setTextEdit(QPointer<QTextEdit> textEdit) {
    m_textEdit = textEdit;
}
