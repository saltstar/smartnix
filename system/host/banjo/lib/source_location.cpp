
#include "banjo/source_location.h"

namespace banjo {

StringView SourceLocation::SourceLine(SourceFile::Position* position_out) const {
    return source_file_->LineContaining(data(), position_out);
}

std::string SourceLocation::position() const {
    std::string position(source_file_->filename());
    SourceFile::Position pos;
    SourceLine(&pos);
    position.push_back(':');
    position.append(std::to_string(pos.line));
    position.push_back(':');
    position.append(std::to_string(pos.column));
    return position;
}

} // namespace banjo
