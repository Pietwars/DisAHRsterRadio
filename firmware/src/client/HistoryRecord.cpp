#include "HistoryRecord.h"

void HistoryRecord::receive(struct Datagram datagram, size_t len)
{
    if (history && datagram.type != 'b' && datagram.type != 'B') // Brett hat eigene Ablage
    {
        history->record(datagram, len);
    }
}
