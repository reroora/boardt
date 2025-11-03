#include "board.h"

Board::Board(QObject* parent) : QObject(parent) {}

Board::~Board() {}

void Board::setRegisterMap(RegisterMap map) {
    m_registerMap = map;
}

Board::RegisterMap Board::getRegisterMap() const
{
    return m_registerMap;
}
