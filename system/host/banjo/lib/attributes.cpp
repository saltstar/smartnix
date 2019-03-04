
#include "banjo/attributes.h"

namespace banjo {

bool HasSimpleLayout(const flat::Decl* decl) {
    return decl->GetAttribute("Layout") == "Simple";
}

bool IsDefaultProtocol(const flat::Decl* decl) {
    return decl->GetAttribute("Layout") == "ddk-protocol" &&
           decl->HasAttribute("DefaultProtocol");
}

} // namespace banjo
