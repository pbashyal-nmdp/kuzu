#include "storage/local_storage/local_storage.h"

#include "main/client_context.h"
#include "storage/local_storage/local_node_table.h"
#include "storage/local_storage/local_rel_table.h"
#include "storage/local_storage/local_table.h"
#include "storage/storage_manager.h"
#include "storage/store/table.h"

using namespace kuzu::common;
using namespace kuzu::transaction;

namespace kuzu {
namespace storage {

LocalTable* LocalStorage::getLocalTable(table_id_t tableID, NotExistAction action) {
    if (!tables.contains(tableID)) {
        switch (action) {
        case NotExistAction::CREATE: {
            const auto table = clientContext.getStorageManager()->getTable(tableID);
            switch (table->getTableType()) {
            case TableType::NODE: {
                tables[tableID] = std::make_unique<LocalNodeTable>(*table);
            } break;
            case TableType::REL: {
                tables[tableID] = std::make_unique<LocalRelTable>(*table);
            } break;
            default:
                KU_UNREACHABLE;
            }
        } break;
        case NotExistAction::RETURN_NULL: {
            return nullptr;
        }
        default:
            KU_UNREACHABLE;
        }
    }
    return tables.at(tableID).get();
}

void LocalStorage::commit() {
    for (auto& [tableID, localTable] : tables) {
        if (localTable->getTableType() == TableType::NODE) {
            const auto table = clientContext.getStorageManager()->getTable(tableID);
            table->commit(clientContext.getTx(), localTable.get());
        }
    }
    for (auto& [tableID, localTable] : tables) {
        if (localTable->getTableType() == TableType::REL) {
            const auto table = clientContext.getStorageManager()->getTable(tableID);
            table->commit(clientContext.getTx(), localTable.get());
        }
    }
}

void LocalStorage::rollback() {
    for (auto& [tableID, localTable] : tables) {
        const auto table = clientContext.getStorageManager()->getTable(tableID);
        table->rollback(localTable.get());
    }
}

} // namespace storage
} // namespace kuzu
