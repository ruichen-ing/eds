#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
//    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats {};

    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->SO[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(auto trg : g->trgBitMap){
        if(trg) stats.noIn++;
    }

    return stats;
}

/**
 * Select all edges from a graph with a given edge label.
 * @param projectLabel Label to select.
 * @param outLabel Label to rename the selected edge labels to (used in the TC).
 * @param inverse Follow the edges in inverse direction.
 * @param in The graph to select from.
 * @return The graph which only has edges with the specified edge label.
 */
std::shared_ptr<SimpleGraph> SimpleEvaluator::selectLabel(uint32_t projectLabel, uint32_t outLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());
    if(!inverse){
        auto buffer = in->PSO[projectLabel];
        for(auto pairs : buffer){
            if(!out->edgeExists(pairs.first, pairs.second, outLabel)) {
                out->addEdge(pairs.first, pairs.second, outLabel);
            }
        }
    }else{
        auto buffer = in->POS[projectLabel];
        for(auto pairs : buffer){
            if(!out->edgeExists(pairs.first, pairs.second, outLabel)) {
                out->addEdge(pairs.first, pairs.second, outLabel);
            }
        }
    }

    return out;
}

/**
 * A SMART transitive closure (TC) computation.
 * @param in Input graph
 * @return
 */
std::shared_ptr<SimpleGraph> SimpleEvaluator::transitiveClosure(std::shared_ptr<SimpleGraph> &base) {

    auto out = std::make_shared<SimpleGraph>(base);
    auto delta = std::make_shared<SimpleGraph>(base);

    uint32_t numNewAdded = 1;
    while (numNewAdded) {
        delta = join(delta, out);
        numNewAdded = unionDistinctTC(out, delta);
    }

    return out;
}

/**
 * Merges a graph into another graph, used for computation of kleene star.
 * @param left A graph to be merged into.
 * @param right A graph to be merged from.
 * @return A number of distinct new edges added from the "right" graph into the "left" graph.
 */
std::shared_ptr<SimpleGraph> SimpleEvaluator::unionDistinct(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    for(uint32_t source = 0; source < right->getNoVertices(); source++) {
        for (auto target : right->SO[source]) {

            if(!left->edgeExists(source, target, 0)) {
                left->addEdge(source, target, 0);
            }
        }
    }
    return left;
}

/**
 * Merges a graph into another graph, used for transitive closure.
 * @param left A graph to be merged into.
 * @param right A graph to be merged from.
 * @return A number of distinct new edges added from the "right" graph into the "left" graph.
 */
uint32_t SimpleEvaluator::unionDistinctTC(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    uint32_t numNewAdded = 0;

    auto newDelta = std::make_shared<SimpleGraph>(left->getNoVertices());

    for(uint32_t source = 0; source < right->getNoVertices(); source++) {
        for (auto target : right->SO[source]) {

            if(!left->edgeExists(source, target, 0)) {
                left->addEdge(source, target, 0);
                newDelta->addEdge(source, target, 0);
                numNewAdded++;
            }
        }
    }

    right = newDelta;

    return numNewAdded;
}


/**
 * Implementation of a join in transitive closure, the same as the default join(), may be removed if the joins in TC can be cached.
 * @param left A graph to be joined.
 * @param right Another graph to join with.
 * @return Answer graph for a join. Note that all labels in the answer graph are "0".
 */
std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto target : left->SO[leftSource]) {

            // try to join the left target with right s
            for (auto rightLabelTarget : right->SO[target]) {

                auto rightTarget = rightLabelTarget;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluateUnionKleene(PathEntry &pe) {

    if(pe.kleene) {
        // evaluate closure
        pe.kleene = false;
        auto base = evaluateUnionKleene(pe);
        return transitiveClosure(base);
    } else {
        // not Kleene
        if (pe.labels.size() == 1) {
            // base label selection
            auto labelDir = pe.labels[0];
            return selectLabel(labelDir.label, 0, labelDir.reverse, graph);
        } else {
            // (left-deep) union
            std::shared_ptr<SimpleGraph> out;
            std::string query = std::to_string(pe.labels[0].label);
            for (int i = 1; i < pe.labels.size(); i++) {
                query += "|" + std::to_string(pe.labels[i].label);
                if(unionCache.count(query) > 0){
                    out = unionCache[query];
                }else{
                    auto right = selectLabel(pe.labels[i].label, 0, pe.labels[0].reverse, graph);
                    if(i == 1){
                        auto left = selectLabel(pe.labels[0].label, 0, pe.labels[0].reverse, graph);
                        out = unionDistinct(left, right);
                    }else{
                        out = unionDistinct(out, right);
                    }
                    unionCache[query] = out;
                }
            }
            return out;
        }
    }
}

/**
 * Given an AST, evaluate the query and produce an answer graph.
 * @param q Parsed AST.
 * @return Solution as a graph.
 */
std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluateConcat(std::vector<PathEntry> &path, std::string query) {
    auto out  = evaluateUnionKleene(path[0]);
    // evaluate according to the AST top-down
    // 1. evaluate concatenations
    std::string subquery;
    if(path.size() > 1) {
        query = query.substr (query.find(",") + 1, query.rfind(",") - 2);
        uint32_t slash = query.find("/");
        slash = query.find("/", slash + 1);
        if(slash > query.length()){
            subquery = query;
            query = "";
        }else{
            subquery = query.substr (0, slash);
            query = query.substr(slash + 1);
        }
        // (left-deep) join
        for(int i = 1; i < path.size(); i++) {
            if(joinCache.count(subquery) > 0){
                out = joinCache[subquery];
            }else{
                auto rg  = evaluateUnionKleene(path[i]);
                out = join(out, rg);
                joinCache[subquery] = out;
            }
            slash = query.find("/");
            if(!query.empty()){
                if(slash > query.length()){
                    subquery += query;
                    query = "";
                }else{
                    subquery += query.substr (0, slash);
                    query = query.substr(slash + 1);
                }
            }
        }
        return out;
    }
    return out;
}

/**
 * Perform a selection on a source constant.
 * @param s A source constant.
 * @param in A graph to select from.
 * @return An answer graph as a result of the given selection.
 */
std::shared_ptr<SimpleGraph> selectSource(Identifier &s, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    for (auto labelTarget : in->SO[s]) {

        auto target = labelTarget;

        out->addEdge(s, target, 0);
    }

    return out;
}

/**
 * Perform a selection on a target constant.
 * @param s A target constant.
 * @param in A graph to select from.
 * @return An answer graph as a result of the given selection.
 */
std::shared_ptr<SimpleGraph> selectTarget(Identifier &t, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    for (int i = 0; i < in->SO.size(); ++i) {
        for(auto trg : in->SO[i]){
            if(trg == t){
                out->addEdge(i, t, 0);
            }
        }
    }

    return out;
}

/**
 * Evaluate a path query. Produce a cardinality of the answer graph.
 * @param query Query to evaluate.
 * @return A cardinality statistics of the answer graph.
 */
cardStat SimpleEvaluator::evaluate(Triple &query) {
    auto res = evaluateConcat(query.path, query.toString());
    if(query.src != NO_IDENTIFIER) res = selectSource(query.src, res);
    else if(query.trg != NO_IDENTIFIER) res = selectTarget(query.trg, res);
    return SimpleEvaluator::computeStats(res);
}