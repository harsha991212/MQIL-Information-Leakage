
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <random>

using namespace std;
namespace fs = std::filesystem;

struct Program {
    string category;
    vector<string> code;
    int expectedLeak;
    int branches;
    int loops;
};

int main() {

    cout << "========================================\n";
    cout << "       MQIL DATASET GENERATOR\n";
    cout << "========================================\n\n";

    string datasetDir = "../dataset";
    string metadataFile = "../metadata.csv";

    fs::create_directories(datasetDir);

    ofstream metadata(metadataFile);

    if (!metadata.is_open()) {
        cout << "ERROR: Cannot create metadata.csv\n";
        return 1;
    }

    metadata << "program,category,expected_leak,branches,loops\n";

    mt19937 rng(random_device{}());

    uniform_int_distribution<int> valueDist(1, 100);
    uniform_int_distribution<int> locDist(1900,2100);
    uniform_int_distribution<int> fillerDist(1, 100000);

    vector<pair<string, int>> categories = {
        {"no_leak", 250},
        {"direct_leak", 125},
        {"indirect_leak", 125},
        {"branch_leak", 125},
        {"loop_leak", 125},
        {"arithmetic_leak", 125},
        {"complex_mixed", 125}
    };

    int total = 0;

    for (const auto& item : categories) {

        string category = item.first;
        int count = item.second;

        for (int i = 0; i < count; i++) {

            int secret = valueDist(rng);
            int publicValue = valueDist(rng);

            int targetLines = locDist(rng);

            Program p;

            p.category = category;
            p.expectedLeak = (category == "no_leak") ? 0 : 1;
            p.branches = 0;
            p.loops = 0;

            // Start the C program.
            p.code.push_back("int main() {");

            // Common variables.
            p.code.push_back(
                "    int secret = " + to_string(secret) + ";"
            );

            p.code.push_back(
                "    int public = " + to_string(publicValue) + ";"
            );

            p.code.push_back("    int output = 0;");

            // Category-specific leakage logic.
            if (category == "no_leak") {

                p.code.push_back("    output = public + 10;");

            }

            else if (category == "direct_leak") {

                p.code.push_back("    output = secret;");

            }

            else if (category == "indirect_leak") {

                p.code.push_back("    int temp = secret;");
                p.code.push_back("    output = temp;");

            }

            else if (category == "branch_leak") {

                p.branches = 1;

                p.code.push_back("    if (secret > public) {");
                p.code.push_back("        output = 1;");
                p.code.push_back("    } else {");
                p.code.push_back("        output = 0;");
                p.code.push_back("    }");

            }

            else if (category == "loop_leak") {

                p.loops = 1;

                p.code.push_back("    int i = 0;");

                p.code.push_back("    while (i < secret) {");
                p.code.push_back("        output = output + 1;");
                p.code.push_back("        i++;");
                p.code.push_back("    }");

            }

            else if (category == "arithmetic_leak") {

                p.code.push_back("    int temp = secret + public;");
                p.code.push_back("    output = temp * 2;");

            }

            else if (category == "complex_mixed") {

                p.branches = 1;
                p.loops = 1;

                p.code.push_back("    int temp = secret;");

                p.code.push_back("    if (secret > public) {");
                p.code.push_back("        output = temp + public;");
                p.code.push_back("    } else {");
                p.code.push_back("        output = temp * 2;");
                p.code.push_back("    }");

                p.code.push_back(
                    "    for (int i = 0; i < secret; i++) {"
                );

                p.code.push_back("        output = output + i;");
                p.code.push_back("    }");

            }

            // Additional statements to increase LOC.
            // These statements depend only on public data.
            p.code.push_back("    long long work = public;");

            int fillerNumber = 0;

            // Reserve three lines for final output update,
            // return statement and closing brace.
            while ((int)p.code.size() < targetLines - 3) {

                int randomValue = fillerDist(rng);

                string statement =
                    "    work = (work * 31 + public + "
                    + to_string(randomValue)
                    + ") % 1000003;";

                p.code.push_back(statement);

                fillerNumber++;
            }

            // Final statements.
            p.code.push_back(
                "    output = output + (int)(work % 97);"
            );

            p.code.push_back("    return output;");
            p.code.push_back("}");

            // Generate filename.
            total++;

            string number = to_string(total);

            string filename =
                "test_" + string(4 - number.length(), '0')
                + number + "_" + category + ".c";

            // Write generated program.
            ofstream outputFile(datasetDir + "/" + filename);

            if (!outputFile.is_open()) {
                cout << "ERROR creating " << filename << "\n";
                continue;
            }

            for (const string& line : p.code) {
                outputFile << line << "\n";
            }

            outputFile.close();

            // Write metadata.
            metadata
                << filename << ","
                << p.category << ","
                << p.expectedLeak << ","
                << p.branches << ","
                << p.loops << "\n";

            if (total % 100 == 0) {
                cout << total << " programs generated...\n";
            }

        }
    }

    metadata.close();

    cout << "\n========================================\n";
    cout << "       MQIL DATASET GENERATION DONE\n";
    cout << "========================================\n";

    cout << total << " C programs generated successfully.\n";

    cout << "Each program contains between "
         << "1900 and 2100 lines.\n";

    cout << "========================================\n";

    return 0;
}

