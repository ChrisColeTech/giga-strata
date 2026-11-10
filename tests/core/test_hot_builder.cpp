#include "moe/hot_builder.h"

#include "common/constants.h"
#include "gguf/model_layout.h"

#include <QtTest>

#include <cmath>

// Expected values come from moe_hot.py (CPython 3.12) on the same layout and counts; the wide
// randomized comparison lives in the out-of-repo harness that drives tests/tools/moe_cli.
namespace {

constexpr qint64 MIB = constants::MIB;

ModelLayout tinyLayout() {
    ModelLayout layout;
    layout.layerCount = 4;
    layout.expertCount = 4;
    layout.expertUsedCount = 2;
    layout.expertBytes = {0, 4 * 8 * MIB, 4 * 8 * MIB, 4 * 2 * MIB};
    return layout;
}

UsageCounts tinyCounts() {
    return {{1, {10.0, 40.0, 30.0, 20.0}}, {2, {5.0, 5.0, 5.0, 5.0}}, {3, {1.0, 0.0, 0.0, 3.0}}};
}

const QVector<int> STARTS{0, 2};
const QVector<int> SIZES{2, 2};

} // namespace

class TestHotBuilder : public QObject {
    Q_OBJECT

private slots:
    void greedyFillsEachGpuByReadsPerByte() {
        const HotMap hot = HotBuilder::buildGreedy(tinyLayout(), tinyCounts(), STARTS, SIZES, {20 * MIB, 9 * MIB});
        QCOMPARE(hot, HotMap({{1, {1, 2}}, {2, {}}, {3, {0, 1, 2, 3}}}));
        QCOMPARE(HotBuilder::predictMs(tinyLayout(), tinyCounts(), hot, {0, 3}, 29.7, 0.29), 1.975446127946128);
    }

    void costKnapsackMatchesTheReference() {
        const auto mixed = HotBuilder::buildCost(tinyLayout(), tinyCounts(), STARTS, SIZES, {20 * MIB, 9 * MIB},
                                                 29.7, 0.29, true);
        QCOMPARE(mixed.first, HotMap({{1, {1, 2}}, {2, {}}, {3, {0, 1, 2, 3}}}));
        QCOMPARE(mixed.second, 1.263922558922559);
        const QVector<int> order = HotBuilder::costInsertionOrder(mixed.first, STARTS, SIZES);
        QCOMPARE(order, QVector<int>({1, 3, 2}));
        QCOMPARE(HotBuilder::hitRate(tinyLayout(), tinyCounts(), mixed.first, order), 0.4222222222222222);

        const auto whole = HotBuilder::buildCost(tinyLayout(), tinyCounts(), STARTS, SIZES, {20 * MIB, 9 * MIB},
                                                 29.7, 0.29, false);
        QCOMPARE(whole.first, HotMap({{1, {}}, {2, {}}, {3, {0, 1, 2, 3}}}));
        QCOMPARE(whole.second, 1.632188552188552);
    }

    void zeroBudgetKeepsEveryLayerCold() {
        const auto cost = HotBuilder::buildCost(tinyLayout(), tinyCounts(), STARTS, SIZES, {0, 0}, 29.7, 0.29, true);
        QCOMPARE(cost.first, HotMap({{1, {}}, {2, {}}, {3, {}}}));
        QCOMPARE(cost.second, 2.053712121212121);
        QCOMPARE(HotBuilder::hitRate(tinyLayout(), tinyCounts(), cost.first), 0.0);
    }

    void cancelReturnsEmptyAndInfinite() {
        const std::atomic<bool> cancel{true};
        const auto cost = HotBuilder::buildCost(tinyLayout(), tinyCounts(), STARTS, SIZES, {20 * MIB, 9 * MIB}, 29.7,
                                                0.29, true, &cancel);
        QVERIFY(cost.first.isEmpty());
        QVERIFY(std::isinf(cost.second));
    }

    void unitsRoundUpLikePythonFloorDivision() {
        QCOMPARE(HotBuilder::unitsFor(0, 3.5 * MIB), qint64(0));
        QCOMPARE(HotBuilder::unitsFor(1, 4.0 * MIB), qint64(1));
        QCOMPARE(HotBuilder::unitsFor(1, 4.0 * MIB + 1), qint64(2));
        QCOMPARE(HotBuilder::unitsFor(3, 14155776.0), qint64(11));
    }
};

QTEST_APPLESS_MAIN(TestHotBuilder)
#include "test_hot_builder.moc"
