#pragma once

#include <QMap>
#include <QVector>

// moe_hot counts: layer index -> how often each expert of that layer was routed to.
// Doubles because the Python reference sums them as floats.
using UsageCounts = QMap<int, QVector<double>>;
