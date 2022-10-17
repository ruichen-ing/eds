#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H

#include <memory>
#include <cmath>
#include "SimpleGraph.h"
#include "Query.h"
#include "Evaluator.h"
#include "Graph.h"

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;
    std::unordered_map<std::string, std::shared_ptr<SimpleGraph>> joinCache;
    std::unordered_map<std::string, std::shared_ptr<SimpleGraph>> unionCache;

public:

    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEvaluator() = default;

    void prepare() override ;
    cardStat evaluate(Triple &query) override ;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<SimpleGraph> evaluateConcat(std::vector<PathEntry> &path, std::string query);
    std::shared_ptr<SimpleGraph> evaluateUnionKleene(PathEntry &pe);

    static std::shared_ptr<SimpleGraph> selectLabel(uint32_t projectLabel, uint32_t outLabel, bool inverse, std::shared_ptr<SimpleGraph> &in);
    static std::shared_ptr<SimpleGraph> join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right);
    static std::shared_ptr<SimpleGraph> transitiveClosure( std::shared_ptr<SimpleGraph> &base);
    static std::shared_ptr<SimpleGraph> unionDistinct(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right);
    static uint32_t unionDistinctTC(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right);
    static cardStat computeStats(std::shared_ptr<SimpleGraph> &g);

};


#endif //QS_SIMPLEEVALUATOR_H
