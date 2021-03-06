// terarkdb_recovery_unit.h

/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include "mongo_terarkdb_common.hpp"

#include <memory.h>


#include "mongo/base/owned_pointer_vector.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/record_id.h"
#include "mongo/db/storage/recovery_unit.h"
#include "mongo/db/storage/snapshot_name.h"
#include "mongo/util/concurrency/ticketholder.h"
#include "mongo/util/timer.h"

namespace mongo { namespace terarkdb {

class TerarkDbRecoveryUnit final : public RecoveryUnit {
public:
    TerarkDbRecoveryUnit();

    virtual ~TerarkDbRecoveryUnit();

    virtual void reportState(BSONObjBuilder* b) const;

    void beginUnitOfWork(OperationContext* opCtx) final;
    void commitUnitOfWork() final;
    void abortUnitOfWork() final;

    virtual bool waitUntilDurable();

    virtual void registerChange(Change* change);

    virtual void abandonSnapshot();

    // un-used API
    virtual void* writingPtr(void* data, size_t len) {
        invariant(!"don't call writingPtr");
		return nullptr; // remove compiler warning
    }

    virtual void setRollbackWritesDisabled() {}

    virtual SnapshotId getSnapshotId() const;

    Status setReadFromMajorityCommittedSnapshot() final;
    bool isReadingFromMajorityCommittedSnapshot() const final {
        return _readFromMajorityCommittedSnapshot;
    }

    boost::optional<SnapshotName> getMajorityCommittedSnapshot() const final;

    // ---- TerarkDb STUFF

    bool inActiveTxn() const {
        return _active;
    }
    void assertInActiveTxn() const;

    bool everStartedWrite() const {
        return _everStartedWrite;
    }

    void setOplogReadTill(const RecordId& id);
    RecordId getOplogReadTill() const {
        return _oplogReadTill;
    }

    void markNoTicketRequired();

    static TerarkDbRecoveryUnit* get(OperationContext* txn);

    static void appendGlobalStats(BSONObjBuilder& b);

    /**
     * Prepares this RU to be the basis for a named snapshot.
     *
     * Begins a TerarkDb transaction, and invariants if we are already in one.
     * Bans being in a WriteUnitOfWork until the next call to abandonSnapshot().
     */
    void prepareForCreateSnapshot(OperationContext* opCtx);

private:
    void _abort();
    void _commit();

    void _ensureSession();
    void _txnClose(bool commit);
    void _txnOpen(OperationContext* opCtx);

    bool _areWriteUnitOfWorksBanned = false;
    bool _inUnitOfWork;
    bool _active;
    uint64_t _myTransactionCount;
    bool _everStartedWrite;
    Timer _timer;
    RecordId _oplogReadTill;
    bool _readFromMajorityCommittedSnapshot = false;
    SnapshotName _majorityCommittedSnapshot = SnapshotName::min();

    typedef OwnedPointerVector<Change> Changes;
    Changes _changes;

    bool _noTicketNeeded;
    void _getTicket(OperationContext* opCtx);
    TicketHolderReleaser _ticket;
};

} } // namespace mongo::terark

