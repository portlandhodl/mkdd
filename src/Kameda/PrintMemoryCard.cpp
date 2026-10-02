#include "Kameda/PrintMemoryCard.h"
#include "Kameda/PrintWindow.h"

PrintMemoryCard::PrintMemoryCard(JKRHeap *) {}

void PrintMemoryCard::reset() {}

void PrintMemoryCard::init(PrintMemoryCard::MessageID msg) {
    _f = 1;
    mMessageID = msg;
    _14 = 2;
    _1c = 2;
}

void PrintMemoryCard::changeMessage() {}

void PrintMemoryCard::draw() {
    if (mMessageID != mcMsg40) {
        mpWindow->draw();
    }
}

void PrintMemoryCard::calc() {}

void PrintMemoryCard::closeWindow() {}

void PrintMemoryCard::closeWindowNoSe() {}

void PrintMemoryCard::setBmgPtr() {}

void PrintMemoryCard::isMessage() {}

void PrintMemoryCard::getChoiceType() {}

void PrintMemoryCard::getWindowSize() {}

void PrintMemoryCard::getWindowColor() {}

#include "JSystem/JAudio/JASFakeMatch2.h"
