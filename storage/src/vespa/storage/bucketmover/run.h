// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \class storage::bucketmover::Run
 * \ingroup storageserver
 *
 * \brief The run class takes care of creating operations for a single run.
 *
 * The run class keeps a matrix that keeps track of up to a max number of
 * buckets that are located on the wrong disk. The first index of the matrix is
 * the source disk and the second index is the ideal disk.
 *
 * The Run does not care about about pending or wait for that. The caller of
 * getNextMove() have to regulate max pending for themselevs.
 */

#pragma once

#include <boost/utility.hpp>
#include <list>
#include <map>
#include <vespa/storage/bucketmover/move.h>
#include <vespa/storage/bucketmover/runstatistics.h>
#include <vespa/vespalib/util/linkedptr.h>
#include <vespa/vdslib/distribution/distribution.h>
#include <vespa/vdslib/state/nodestate.h>

namespace storage {

class MountPointList;
class PartitionMonitor;
class StorBucketDatabase;
class Clock;

namespace bucketmover {

class Run : public document::Printable, boost::noncopyable {
    StorBucketDatabase& _bucketDatabase;
    lib::Distribution::SP _distribution;
    lib::NodeState _nodeState;
    uint16_t _nodeIndex;
    uint32_t _maxEntriesToKeep;
    std::list<Move> _entries;
    std::list<Move> _pending;
    bool _iterationDone;
    RunStatistics _statistics;
    bool _aborted;
    std::map<uint16_t, bool> _diskDisabled;

public:
    Run(StorBucketDatabase&,
        lib::Distribution::SP,
        const lib::NodeState&,
        uint16_t nodeIndex,
        framework::Clock&);

    /**
     * If disk distribution change during runs, they get aborted. We want to
     * track this in run, as we want run to exist until all pending requests
     * have been answered.
     */
    void abort() { _aborted = true; }
    bool aborted() { return _aborted; }

    /**
     * Get next move does the following:
     *   - Sort disks in order of fillrate. (PartitionMonitor keeps cache, so
     *     this will only stat once in a while)
     *   - If the matrix contains a possible move from above average fill rate
     *     to below average fill rate, do that move. Prioritizing moving away
     *     from fullest disk.
     *   - Otherwise, continue visiting bucket database to fill up matrix.
     *   - If any moves left, do next, prioritizing moving away from fullest
     *     disk.
     *
     * @return A Move object. If isDefined() returns false, run is complete.
     *         The whole database have been iterated through.
     */
    Move getNextMove();

    /**
     * Run through the database not doing any moves. Useful to do a run only
     * to gather statistics of current state.
     */
    void depleteMoves();

    void moveOk(Move& move);
    void moveFailedBucketNotFound(Move& move);
    void moveFailed(Move& move);

    const std::list<Move>& getPendingMoves() const { return _pending; }

    RunStatistics& getStatistics() { return _statistics; }
    const RunStatistics& getStatistics() const { return _statistics; }

    virtual void print(std::ostream&, bool verbose,
                       const std::string& indent) const;
private:
    void removePending(Move&);
    void finalize();
};

} // bucketmover
} // storage

