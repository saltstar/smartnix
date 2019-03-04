
#ifndef ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_ERROR_REPORTER_H_
#define ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_ERROR_REPORTER_H_

#include <string>
#include <vector>

#include "source_location.h"
#include "string_view.h"
#include "token.h"

namespace fidl {

class ErrorReporter {
public:
    void ReportError(const SourceLocation& location, StringView message);
    void ReportError(const Token& token, StringView message);
    void ReportError(StringView message);
    void ReportWarning(const SourceLocation& location, StringView message);
    void PrintReports();
    const std::vector<std::string>& errors() const { return errors_; };
    const std::vector<std::string>& warnings() const { return warnings_; };

private:
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
};

} // namespace fidl

#endif // ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_ERROR_REPORTER_H_
