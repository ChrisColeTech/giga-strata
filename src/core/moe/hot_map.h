#pragma once

#include <QMap>
#include <QVector>

// --moe-hot placement: layer index -> sorted expert indices kept in VRAM. An empty list means
// the whole layer stays in RAM, a full list means the whole layer is on the GPU. QMap keeps the
// layers sorted, which is the order write_hot emits them in.
using HotMap = QMap<int, QVector<int>>;
