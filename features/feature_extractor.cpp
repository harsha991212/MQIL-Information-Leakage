#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>

using namespace std;
namespace fs = std::filesystem;


// ------------------------------------------------------------
// Read complete C source file
// ------------------------------------------------------------
string readFile(const string& filePath)
{
    ifstream file(filePath);

    if (!file.is_open())
        return "";

    string content;
    string line;

    while (getline(file, line))
    {
        content += line;
        content += "\n";
    }

    file.close();

    return content;
}


// ------------------------------------------------------------
// Count occurrences using regex pattern
// ------------------------------------------------------------
int countMatches(const string& text, const string& pattern)
{
    try
    {
        regex re(pattern);
        return distance(
            sregex_iterator(text.begin(), text.end(), re),
            sregex_iterator()
        );
    }
    catch (...)
    {
        return 0;
    }
}


// ------------------------------------------------------------
// Count simple keyword occurrences
// ------------------------------------------------------------
int countKeyword(const string& text, const string& keyword)
{
    regex re("\\b" + keyword + "\\b");

    return distance(
        sregex_iterator(text.begin(), text.end(), re),
        sregex_iterator()
    );
}


// ------------------------------------------------------------
// Count lines
// ------------------------------------------------------------
int countLines(const string& text)
{
    if (text.empty())
        return 0;

    int lines = 0;

    for (char c : text)
    {
        if (c == '\n')
            lines++;
    }

    return lines;
}


// ------------------------------------------------------------
// Extract integer value
// Example:
// int secret = 89;
// int public = 58;
// ------------------------------------------------------------
int extractVariable(const string& text, const string& variable)
{
    string pattern =
        "\\bint\\s+" + variable +
        "\\s*=\\s*(-?[0-9]+)";

    regex re(pattern);

    smatch match;

    if (regex_search(text, match, re))
    {
        try
        {
            return stoi(match[1].str());
        }
        catch (...)
        {
            return 0;
        }
    }

    return 0;
}


// ------------------------------------------------------------
// Determine whether secret/public variables exist
// ------------------------------------------------------------
int hasVariable(const string& text, const string& variable)
{
    regex re(
        "\\bint\\s+" + variable +
        "\\s*="
    );

    return regex_search(text, re) ? 1 : 0;
}


// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    cout << "========================================\n";
    cout << "        MQIL FEATURE EXTRACTION\n";
    cout << "========================================\n\n";


    // Dataset is one directory above features/
    string datasetDir = "../dataset";

    // Output CSV
    string outputFile = "features.csv";


    // --------------------------------------------------------
    // Check dataset directory
    // --------------------------------------------------------
    if (!fs::exists(datasetDir))
    {
        cout << "ERROR: Dataset directory not found!\n";
        cout << "Expected location: " << datasetDir << "\n";
        return 1;
    }


    // --------------------------------------------------------
    // Open CSV
    // --------------------------------------------------------
    ofstream csv(outputFile);

    if (!csv.is_open())
    {
        cout << "ERROR: Could not create features.csv\n";
        return 1;
    }


    // --------------------------------------------------------
    // CSV HEADER
    // --------------------------------------------------------
    csv
        << "file_name,"
        << "label,"
        << "lines,"
        << "has_secret,"
        << "has_public,"
        << "if_count,"
        << "else_count,"
        << "for_count,"
        << "while_count,"
        << "switch_count,"
        << "case_count,"
        << "return_count,"
        << "assignment_count,"
        << "comparison_count,"
        << "arithmetic_count,"
        << "array_access_count,"
        << "function_count,"
        << "secret_value,"
        << "public_value,"
        << "direct_secret_use,"
        << "indirect_secret_use,"
        << "loop_secret_use,"
        << "branch_secret_use,"
        << "complexity\n";


    int totalFiles = 0;


    // --------------------------------------------------------
    // Process every .c file
    // --------------------------------------------------------
    for (const auto& entry : fs::directory_iterator(datasetDir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".c")
            continue;


        string filePath = entry.path().string();

        string fileName =
            entry.path().filename().string();


        string code = readFile(filePath);


        if (code.empty())
            continue;


        // ----------------------------------------------------
        // Basic features
        // ----------------------------------------------------
        int lines = countLines(code);

        int hasSecret =
            hasVariable(code, "secret");

        int hasPublic =
            hasVariable(code, "public");


        // ----------------------------------------------------
        // Control-flow features
        // ----------------------------------------------------
        int ifCount =
            countKeyword(code, "if");

        int elseCount =
            countKeyword(code, "else");

        int forCount =
            countKeyword(code, "for");

        int whileCount =
            countKeyword(code, "while");

        int switchCount =
            countKeyword(code, "switch");

        int caseCount =
            countKeyword(code, "case");


        // ----------------------------------------------------
        // Other language features
        // ----------------------------------------------------
        int returnCount =
            countKeyword(code, "return");


        int assignmentCount =
            countMatches(code, "[^=!<>]=[^=]");


        int comparisonCount =
            countMatches(
                code,
                "==|!=|<=|>=|<|>"
            );


        int arithmeticCount =
            countMatches(
                code,
                "\\+|-|\\*|/"
            );


        int arrayAccessCount =
            countMatches(
                code,
                "\\[[^\\]]*\\]"
            );


        // ----------------------------------------------------
        // Function count
        // ----------------------------------------------------
        int functionCount =
            countMatches(
                code,
                "\\b[a-zA-Z_][a-zA-Z0-9_]*\\s*\\([^;{}]*\\)\\s*\\{"
            );


        // ----------------------------------------------------
        // Secret/public values
        // ----------------------------------------------------
        int secretValue =
            extractVariable(code, "secret");

        int publicValue =
            extractVariable(code, "public");


        // ----------------------------------------------------
        // Information-flow related features
        // ----------------------------------------------------

        // Direct use of secret
        int directSecretUse =
            countKeyword(code, "secret");


        // Indirect use:
        // temporary variables assigned from secret
        int indirectSecretUse = 0;

        regex indirectRegex(
            "\\b(int|long|short|float|double)\\s+"
            "[a-zA-Z_][a-zA-Z0-9_]*"
            "\\s*=\\s*secret\\b"
        );

        indirectSecretUse =
            distance(
                sregex_iterator(
                    code.begin(),
                    code.end(),
                    indirectRegex
                ),
                sregex_iterator()
            );


        // ----------------------------------------------------
        // Secret inside loops
        // ----------------------------------------------------
        int loopSecretUse = 0;

        regex loopRegex(
            "(for|while)\\s*\\([^\\)]*secret[^\\)]*\\)"
        );

        loopSecretUse =
            distance(
                sregex_iterator(
                    code.begin(),
                    code.end(),
                    loopRegex
                ),
                sregex_iterator()
            );


        // ----------------------------------------------------
        // Secret inside branches
        // ----------------------------------------------------
        int branchSecretUse = 0;

        regex branchRegex(
            "(if|else\\s+if)\\s*\\([^\\)]*secret[^\\)]*\\)"
        );

        branchSecretUse =
            distance(
                sregex_iterator(
                    code.begin(),
                    code.end(),
                    branchRegex
                ),
                sregex_iterator()
            );


        // ----------------------------------------------------
        // Simple complexity score
        // ----------------------------------------------------
        int complexity =
            1 +
            ifCount +
            forCount +
            whileCount +
            switchCount;


        // ----------------------------------------------------
        // Determine label
        // ----------------------------------------------------
        string label = "unknown";

if (fileName.find("no_leak") != string::npos)
{
    label = "no_leak";
}
else if (fileName.find("indirect_leak") != string::npos)
{
    label = "indirect_leak";
}
else if (fileName.find("direct_leak") != string::npos)
{
    label = "direct_leak";
}
else if (fileName.find("branch_leak") != string::npos)
{
    label = "branch_leak";
}
else if (fileName.find("loop_leak") != string::npos)
{
    label = "loop_leak";
}
else if (fileName.find("arithmetic_leak") != string::npos)
{
    label = "arithmetic_leak";
}
else if (fileName.find("complex_mixed") != string::npos)
{
    label = "complex_mixed";
}

        // ----------------------------------------------------
        // Write CSV row
        // ----------------------------------------------------
        csv
            << fileName << ","
            << label << ","cl /EHsc /std:c++17 feature_extractor.cpp
.\feature_extractor.exe
            << lines << ","
            << hasSecret << ","
            << hasPublic << ","
            << ifCount << ","
            << elseCount << ","
            << forCount << ","
            << whileCount << ","
            << switchCount << ","
            << caseCount << ","
            << returnCount << ","
            << assignmentCount << ","
            << comparisonCount << ","
            << arithmeticCount << ","
            << arrayAccessCount << ","
            << functionCount << ","
            << secretValue << ","
            << publicValue << ","
            << directSecretUse << ","
            << indirectSecretUse << ","
            << loopSecretUse << ","
            << branchSecretUse << ","
            << complexity
            << "\n";


        totalFiles++;


        // Progress every 100 files
        if (totalFiles % 100 == 0)
        {
            cout
                << totalFiles
                << " programs processed...\n";
        }
    }


    csv.close();


    // --------------------------------------------------------
    // Final message
    // --------------------------------------------------------
    cout << "\n";
    cout << "========================================\n";
    cout << "       MQIL FEATURE EXTRACTION DONE\n";
    cout << "========================================\n";

    cout
        << totalFiles
        << " C programs processed successfully.\n";

    cout
        << "Output file: features.csv\n";

    cout << "========================================\n";


    return 0;
}