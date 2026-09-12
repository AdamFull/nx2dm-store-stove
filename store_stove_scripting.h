#pragma once

namespace nxe::script {
class Host;
}

namespace nxm::store_stove {

class StovePCBang;

/// The Stove-only `host.store_stove_*` surface (PC Bang detection) -
/// deliberately separate from store/store_scripting.cpp, which stays
/// neutral-only. Captured by direct reference rather than looked up
/// through ServiceRegistry: nothing outside store_stove itself will ever
/// need to find it, so there's no "which backend provides this" question
/// to resolve.
void expose_store_stove_extras(nxe::script::Host &host, StovePCBang &pcbang);

}
