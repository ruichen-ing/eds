#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"
#include <set>

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

private:
    uint32_t numSrc = 0;
    uint32_t numTrg = 0;
    uint32_t numLabels = 0;
    uint32_t numTuples = 0;
    uint32_t numEdges = 0;

    cardStat unionPairwise(cardStat src, cardStat trg);

    std::vector<std::vector<uint32_t>> labels;

    struct labelInfo{
        std::set<uint32_t> outLabelNode;
        uint32_t numLabelPath = 0;
        std::set<uint32_t> inLabelNode;
    };

public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(Triple &q) override ;

    void estimateOnNodes(uint32_t src, cardStat &estimate, uint32_t trg);

//    cardStat singleOperation(uint32_t reverse, bool kleene);

    cardStat concatenation(cardStat src, cardStat trg);

    cardStat Union(std::vector<LabelDir> labels, bool kleene);

    cardStat transClosure(std::vector<LabelDir> labels, bool kleene);

    cardStat singleOperation(uint32_t reverse, uint32_t label, bool kleene);

};


#endif //QS_SIMPLEESTIMATOR_H
