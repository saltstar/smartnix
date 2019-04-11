#include <lib/system-topology.h>
#include <zircon/errors.h>

namespace system_topology {
namespace {
constexpr size_t kMaxTopologyDepth = 20;

inline void ValidationError(int index, const char* message) {
    printf("Error validating topology at node %d : %s \n", index, message);
}

} // namespace

zx_status_t Graph::Update(zbi_topology_node_t* flat_nodes, size_t count) {
    if (flat_nodes == nullptr || count == 0 || !Validate(flat_nodes, static_cast<int>(count))) {
        return ZX_ERR_INVALID_ARGS;
    }

    if (nodes_.get() != nullptr) {
        return ZX_ERR_ALREADY_EXISTS;
    }

    fbl::AllocChecker checker;
    nodes_.reset(new (&checker) Node[count]{{}});
    if (!checker.check()) {
        return ZX_ERR_NO_MEMORY;
    }

    Node* node = nullptr;
    zbi_topology_node_t* flat_node = nullptr;
    for (size_t i = 0; i < count; ++i) {
        flat_node = &flat_nodes[i];
        node = &nodes_[i];

        node->entity_type = flat_node->entity_type;

        // Copy info.
        switch (node->entity_type) {
        case ZBI_TOPOLOGY_ENTITY_PROCESSOR:
            node->entity.processor = flat_node->entity.processor;

            processors_.push_back(node, &checker);
            if (!checker.check()) {
                nodes_.reset(nullptr);
                return ZX_ERR_NO_MEMORY;
            }

            for (int i = 0; i < node->entity.processor.logical_id_count; ++i) {
                processors_by_logical_id_.insert(node->entity.processor.logical_ids[i],
                                                 node, &checker);
                if (!checker.check()) {
                    nodes_.reset(nullptr);
                    return ZX_ERR_NO_MEMORY;
                }
            }

            break;
        case ZBI_TOPOLOGY_ENTITY_CLUSTER:
            node->entity.cluster = flat_node->entity.cluster;
            break;
        case ZBI_TOPOLOGY_ENTITY_NUMA_REGION:
            node->entity.numa_region = flat_node->entity.numa_region;
            break;
        default:
            // Other types don't have attached info.
            break;
        }

        if (flat_node->parent_index != ZBI_TOPOLOGY_NO_PARENT) {
            // Validation should have prevented this.
            ZX_DEBUG_ASSERT_MSG(flat_node->parent_index >= 0 && flat_node->parent_index < count,
                                "parent_index out of range: %u\n", flat_node->parent_index);

            node->parent = &nodes_[flat_node->parent_index];
            node->parent->children.push_back(node, &checker);
            if (!checker.check()) {
                nodes_.reset(nullptr);
                return ZX_ERR_NO_MEMORY;
            }
        }
    }

    return ZX_OK;
}

bool Graph::Validate(zbi_topology_node_t* nodes, int count) const {
    uint16_t parents[kMaxTopologyDepth];
    for (size_t i = 0; i < kMaxTopologyDepth; ++i) {
        parents[i] = ZBI_TOPOLOGY_NO_PARENT;
    }

    uint8_t current_type = ZBI_TOPOLOGY_ENTITY_UNDEFINED;
    int current_depth = 0;

    zbi_topology_node_t* node;
    for (int current_index = count - 1; current_index >= 0; current_index--) {
        node = &nodes[current_index];

        if (current_type == ZBI_TOPOLOGY_ENTITY_UNDEFINED) {
            current_type = node->entity_type;
        }

        if (current_type != node->entity_type) {

            if (current_index == parents[current_depth]) {
                // If the type changes then it should be the parent of the
                // previous level.
                current_depth++;

                if (current_depth == kMaxTopologyDepth) {
                    ValidationError(current_index,
                                    "Structure is too deep, we only support 20 levels.");
                    return false;
                }
            } else if (node->entity_type == ZBI_TOPOLOGY_ENTITY_PROCESSOR) {
                // If it isn't the parent of the previous level, but it is a process than we have
                // encountered a new branch and should start walking from the bottom again.

                // Clear the parent index's for all levels but the top, we want to ensure that the
                // top level of the new branch reports to the same parent as we do.
                for (int i = current_depth - 1; i >= 0; --i) {
                    parents[i] = ZBI_TOPOLOGY_NO_PARENT;
                }
                current_depth = 0;
            } else {
                // Otherwise the structure is incorrect.
                ValidationError(current_index,
                                "Graph is not stored in correct order, with children adjacent to "
                                "parents");
                return false;
            }
            current_type = node->entity_type;
        }

        if (parents[current_depth] == ZBI_TOPOLOGY_NO_PARENT) {
            parents[current_depth] = node->parent_index;
        } else if (parents[current_depth] != node->parent_index) {
            ValidationError(current_index, "Parents at level do not match.");
            return false;
        }

        // Ensure that all leaf nodes are processors.
        if (current_depth == 0 && node->entity_type != ZBI_TOPOLOGY_ENTITY_PROCESSOR) {
            ValidationError(current_index, "Encountered a leaf node that isn't a processor.");
            return false;
        }

        // Ensure that all processors are leaf nodes.
        if (current_depth != 0 && node->entity_type == ZBI_TOPOLOGY_ENTITY_PROCESSOR) {
            ValidationError(current_index, "Encountered a processor that isn't a leaf node.");
            return false;
        }

        // By the time we reach the first parent we should be at the maximum depth and have no
        // parents defined.
        if (current_index == 0 && parents[current_depth] != ZBI_TOPOLOGY_NO_PARENT &&
            (current_depth == kMaxTopologyDepth - 1 ||
             parents[current_depth + 1] == ZBI_TOPOLOGY_NO_PARENT)) {
            ValidationError(current_index, "Top level of tree should not have a parent");
            return false;
        }
    }
    return true;
}

} // namespace system_topology
