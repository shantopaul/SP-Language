#include <gtest/gtest.h>
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/diagnostics/SourceLocation.h"
#include <sstream>

namespace sp::diagnostics::test {

TEST(DiagnosticEngineTest, ReportsErrorsAndWarningsAccurately) {
    std::ostringstream oss;
    DiagnosticEngine engine(oss);

    EXPECT_FALSE(engine.hasErrors());
    EXPECT_EQ(engine.errorCount(), 0);
    EXPECT_EQ(engine.warningCount(), 0);

    engine.reportError("E1001", "Type mismatch in variable assignment", SourceLocation("main.sp", 8, 14), "Cast to int");
    EXPECT_TRUE(engine.hasErrors());
    EXPECT_EQ(engine.errorCount(), 1);
    EXPECT_EQ(engine.warningCount(), 0);

    engine.reportWarning("W2001", "Unused variable", SourceLocation("main.sp", 15, 5));
    EXPECT_EQ(engine.errorCount(), 1);
    EXPECT_EQ(engine.warningCount(), 1);

    std::string output = oss.str();
    EXPECT_NE(output.find("E1001"), std::string::npos);
    EXPECT_NE(output.find("Type mismatch"), std::string::npos);
    EXPECT_NE(output.find("main.sp:8:14"), std::string::npos);
    EXPECT_NE(output.find("W2001"), std::string::npos);

    engine.clear();
    EXPECT_FALSE(engine.hasErrors());
    EXPECT_EQ(engine.errorCount(), 0);
    EXPECT_EQ(engine.warningCount(), 0);
}

} // namespace sp::diagnostics::test
