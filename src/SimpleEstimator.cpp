#include <set>
#include "SimpleGraph.h"
#include "SimpleEstimator.h"
#include  <cmath>

#define MAX_UINT32_T 0xffffffff

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
}

bool cmp(const std::pair<uint32_t,uint32_t> &a, const std::pair<uint32_t,uint32_t> &b) {
    if (a.first == b.first)
        return a.second < b.second;
    if (a.first < b.first)
        return true;
    return false;
}

void SimpleEstimator::prepare() {
    // do your prep here

    // generate statistics from the graph
//    numEdges = graph->getNoEdges();
//    numTuples = graph->getNoDistinctEdges();
//    numLabels = graph->getNoLabels();
//
//    // count: how many out-nodes, in-nodes and paths each label has
//    std::vector<labelInfo> buffer(numLabels);
//    for (int i = 0; i < graph->getNoVertices(); ++i) {
//        for(auto node : graph->adj[i]){
//            buffer[node.first].numLabelPath++;
//            buffer[node.first].outLabelNode.insert(i);
//            buffer[node.first].inLabelNode.insert(node.second);
//        }
//    }
//
//    labels = std::vector<std::vector<uint32_t>>(numLabels);
//    for (int i = 0; i < numLabels; ++i) {
//        labels[i].push_back(buffer[i].outLabelNode.size());
//        labels[i].push_back(0);
//        labels[i].push_back(buffer[i].inLabelNode.size());
//    }
//
//    for (auto nodeGraph : graph->adj) {
//        std::sort(nodeGraph.begin(), nodeGraph.end(), cmp);
//        uint32_t prevTrg = 0;
//        uint32_t prevLabel = 0;
//        bool first = true;
//        for (auto pair : nodeGraph) {
//            if (first || !(prevTrg == pair.second && prevLabel == pair.first)) {
//                first = false;
//                prevTrg = pair.second;
//                prevLabel = pair.first;
//                labels[prevLabel][1]++;
//            }
//        }
//    }
//
//    // number of distinct src nodes
//    for(auto nodeVector : graph->adj){
//        if(nodeVector.size() > 0){
//            numSrc++;
//        }
//    }
//
//    // number of distinct trg nodes
//    // reverse_adj: reversed adjacency list: trgNode->(label1, srcNode1)->(label2, srcNode2)...
//    for(auto nodeVector : graph->reverse_adj){
//        if(nodeVector.size() > 0){
//            numTrg++;
//        }
//    }
}

cardStat SimpleEstimator::estimate(Triple &q) {

    // perform your estimation here

    // parse the triple and perform corresponding operations(single selection, join, kleene, union, transitive closure)
    cardStat src{0, 0, 0};
    cardStat trg{0, 0, 0};

    if(q.path.size() == 0){
        std::cout << "invalid query" << std::endl;
        return cardStat{0, 0, 0};
    }

    // if there is only one query, then no joins
    if(q.path[0].labels.size() == 1) {
        // (,>,), (,>+,), (,<,), (,<+,)
        src = singleOperation(q.path[0].labels[0].reverse, q.path[0].labels[0].label, q.path[0].kleene);
    }else{
        // union or transitive closure
        if(q.path[0].kleene){
            // (,(>|>|>)+,)
            src = transClosure(q.path[0].labels, true);
        }else{
            // (,(>|>|>),)
            src = Union(q.path[0].labels, false);
        }
    }

    // q.path.size() > 1, then join
    for(int i = 1; i < q.path.size(); ++i){
        // not union or transitive closure
        if(q.path[i].labels.size() == 1) {
            trg = singleOperation(q.path[i].labels[0].reverse, q.path[i].labels[0].label, q.path[i].kleene);
        }else{
            // (,>/(>|>|>)+,)
            if(q.path[i].kleene){
                trg = transClosure(q.path[i].labels, true);
                // (,>/(>|>|>),)
            }else{
                trg = Union(q.path[i].labels, false);
            }
        }
        src = concatenation(src, trg);
    }

    estimateOnNodes(q.src, src, q.trg);
    return src;
}

// estimation based on whether src and/or trg is fixed or unspecified
void SimpleEstimator::estimateOnNodes(uint32_t src, cardStat &estimate, uint32_t trg) {
    if(src != MAX_UINT32_T && trg == MAX_UINT32_T){
        estimate.noPaths = ceil((float)estimate.noPaths / estimate.noOut);
        estimate.noIn = ceil((float)estimate.noIn / estimate.noOut);
        estimate.noOut = 1;
    }else if(src == MAX_UINT32_T && trg != MAX_UINT32_T){
        estimate.noPaths = ceil((float)estimate.noPaths / estimate.noIn);
        estimate.noOut = ceil((float)estimate.noOut / estimate.noIn);
        estimate.noIn = 1;
    }
}

// (,(>|>|>|>)+,)
cardStat SimpleEstimator::transClosure(std::vector<LabelDir> labels, bool kleene) {
    // we assume kleene is equal to single operation, therefore, transitive closure degrades to union
    return Union(labels, kleene);
}

// (,(>|>|>|>),)
cardStat SimpleEstimator::Union(std::vector<LabelDir> labels, bool kleene) {
    cardStat src{0, 0, 0};
    for (int i = 0; i < labels.size(); ++i) {
        if(i == 0){
            src = singleOperation(labels[i].reverse, labels[i].label, kleene);
        }else{
            cardStat trg = singleOperation(labels[i].reverse, labels[i].label, kleene);
            src = unionPairwise(src, trg);
        }
    }
    return src;
}

cardStat SimpleEstimator::unionPairwise(cardStat src, cardStat trg) {
    // min:
    cardStat smaller = src.noPaths > trg.noPaths? src : trg;
    // max:
    uint32_t maxPaths;
    uint32_t maxOut;
    uint32_t maxIn;
    if((src.noPaths + trg.noPaths) > numTuples){
        maxPaths = numTuples / numLabels;
        maxOut = numSrc / numLabels;
        maxIn = numTrg / numLabels;
    }else{
        maxPaths = src.noPaths + trg.noPaths;
        maxOut = src.noOut + trg.noOut;
        maxIn = src.noIn + trg.noIn;
    }
    return cardStat{(smaller.noOut + maxOut) / 2, (smaller.noPaths + maxPaths) / 2, (smaller.noIn + maxIn) / 2};
//    return cardStat{src.noOut + trg.noIn, src.noPaths + trg.noPaths, src.noIn + trg.noIn};
}

// deal with > and <
cardStat SimpleEstimator::singleOperation(uint32_t reverse, uint32_t label, bool kleene) {
    if(reverse){
        return cardStat{labels[label][2], labels[label][1], labels[label][0]};
    }else{
        return cardStat{labels[label][0], labels[label][1], labels[label][2]};
    }
}

// join
cardStat SimpleEstimator::concatenation(cardStat src, cardStat trg) {
    double noPaths = std::min(src.noPaths * trg.noPaths / trg.noOut, trg.noPaths * src.noPaths / src.noIn);
//    uint32_t noPaths = src.noPaths * trg.noPaths / trg.noOut;
    double rf_Src = noPaths / src.noPaths;
    double rf_Trg = noPaths / trg.noPaths;
    uint32_t noOut = src.noOut * rf_Src;
    uint32_t noIn = trg.noIn * rf_Trg;

    return cardStat{noOut, (uint32_t)noPaths, noIn};
}
