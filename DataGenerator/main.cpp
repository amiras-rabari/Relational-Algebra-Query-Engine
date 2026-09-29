#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <filesystem>
#include <cmath>
#include <stdexcept>





namespace fs = std::filesystem;

class DataGenerator {
public:
    struct RTuple {
        int a; // Unique attribute
        int b; // Join attribute
    };

    struct STuple {
        int b; // Join attribute
        int c; // Unique payload
    };

    explicit DataGenerator(unsigned int seed = 42)
        : rng_(seed) {}

    /**
     * Generate:
     *   R(a,b) with exactly numR tuples
     *   S(b,c) with exactly numS tuples
     *
     * matchRate means approximately how many S tuples
     * each R tuple joins with on R.b = S.b.
     *
     * Example:
     *   numR = 1000
     *   numS = 2500
     *   matchRate = 2.0
     *
     * gives approximately:
     *   2000 matching S tuples
     *   500 non-matching S tuples
     *
     * Therefore the average join fanout is about 2 S tuples
     * per R tuple.
     */
    void generateRS(
        int numR,
        int numS,
        double matchRate,
        const std::string& outputDir = "..")
    {
        if (numR <= 0) {
            throw std::invalid_argument(
                "numR must be positive.");
        }

        if (numS <= 0) {
            throw std::invalid_argument(
                "numS must be positive.");
        }

        if (!std::isfinite(matchRate) || matchRate < 0.0) {
            throw std::invalid_argument(
                "matchRate must be finite and >= 0.");
        }

        // Number of S tuples that should actually match R.
        const int matchingSTuples =
            static_cast<int>(std::llround(numR * matchRate));

        // It is impossible to create more matching S tuples
        // than the total number of S tuples.
        if (matchingSTuples > numS) {
            throw std::invalid_argument(
                "matchRate is too large for the requested numR/numS. "
                "Need numR * matchRate <= numS.");
        }

        std::vector<RTuple> rTuples;
        std::vector<STuple> sTuples;

        rTuples.reserve(numR);
        sTuples.reserve(numS);

        /*
         * ---------------------------------------------------------
         * 1. Generate R(a,b)
         * ---------------------------------------------------------
         *
         * Use unique b values:
         *
         *   R(1,1)
         *   R(2,2)
         *   ...
         *   R(numR,numR)
         *
         * This makes it easy to control how many S tuples
         * are associated with each R tuple.
         */
        for (int i = 1; i <= numR; ++i) {
            const int a = i;
            const int b = i;

            rTuples.push_back({a, b});
        }

        /*
         * ---------------------------------------------------------
         * 2. Generate matching S tuples
         * ---------------------------------------------------------
         *
         * Distribute matching S tuples cyclically across
         * the R join keys.
         *
         * Example:
         *
         * numR = 4
         * matchingSTuples = 10
         *
         * S.b values:
         *
         * 1,2,3,4,1,2,3,4,1,2
         *
         * Therefore the match counts are approximately:
         *
         * R1 -> 3 matches
         * R2 -> 3 matches
         * R3 -> 2 matches
         * R4 -> 2 matches
         *
         * Average = 10 / 4 = 2.5
         */
        for (int i = 0; i < matchingSTuples; ++i) {
            const int b = (i % numR) + 1;
            const int c = 100000 + i + 1;

            sTuples.push_back({b, c});
        }

        /*
         * ---------------------------------------------------------
         * 3. Generate non-matching S tuples
         * ---------------------------------------------------------
         *
         * These b values are outside the R.b range,
         * so they cannot join with any R tuple.
         *
         * R.b is [1 ... numR]
         *
         * Non-matching S.b starts at numR + 1.
         */
        const int nonMatchingSTuples =
            numS - matchingSTuples;

        for (int i = 0; i < nonMatchingSTuples; ++i) {
            const int b = numR + i + 1;
            const int c =
                100000 + matchingSTuples + i + 1;

            sTuples.push_back({b, c});
        }

        /*
         * ---------------------------------------------------------
         * 4. Shuffle relations
         * ---------------------------------------------------------
         *
         * This prevents the files from being sorted by join key
         * and gives the engine ordinary unsorted relation data.
         */
        std::shuffle(rTuples.begin(), rTuples.end(), rng_);
        std::shuffle(sTuples.begin(), sTuples.end(), rng_);

        /*
         * ---------------------------------------------------------
         * 5. Create output directory
         * ---------------------------------------------------------
         */
        if (!outputDir.empty()) {
            fs::create_directories(outputDir);
        }

        const std::string pathR =
            (fs::path(outputDir) / "R.txt").string();

        const std::string pathS =
            (fs::path(outputDir) / "S.txt").string();

        /*
         * ---------------------------------------------------------
         * 6. Write files
         * ---------------------------------------------------------
         */
        writeR(pathR, rTuples);
        writeS(pathS, sTuples);

        std::cout
            << "[OK] Generated relations\n"
            << "     R tuples:            " << numR << '\n'
            << "     S tuples:            " << numS << '\n'
            << "     Matching S tuples:   " << matchingSTuples << '\n'
            << "     Non-matching S:      " << nonMatchingSTuples << '\n'
            << "     Target match rate:   " << matchRate << '\n'
            << "     Actual average rate:  "
            << static_cast<double>(matchingSTuples) / numR
            << '\n'
            << "     Output directory:    " << outputDir << '\n'
            << "     R file:              " << pathR << '\n'
            << "     S file:              " << pathS << '\n';
    }

private:
    std::mt19937 rng_;

    void writeR(
        const std::string& filepath,
        const std::vector<RTuple>& tuples)
    {
        std::ofstream outFile(filepath);

        if (!outFile) {
            throw std::runtime_error(
                "Failed to open R file: " + filepath);
        }

        outFile << "R (a, b) = {\n";

        for (const auto& t : tuples) {
            outFile << "  "
                    << t.a << ", "
                    << t.b << '\n';
        }

        outFile << "}\n";
    }

    void writeS(
        const std::string& filepath,
        const std::vector<STuple>& tuples)
    {
        std::ofstream outFile(filepath);

        if (!outFile) {
            throw std::runtime_error(
                "Failed to open S file: " + filepath);
        }

        outFile << "S (b, c) = {\n";

        for (const auto& t : tuples) {
            outFile << "  "
                    << t.b << ", "
                    << t.c << '\n';
        }

        outFile << "}\n";
    }
};


int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <numR> <numS> <matchRate> [outputDirectory]\n\n"
            << "Example:\n"
            << "  "
            << argv[0]
            << " 1000 1000 1.0 .\n";

        return 1;
    }

    try {
        const int numR =
            std::stoi(argv[1]);

        const int numS =
            std::stoi(argv[2]);

        const double matchRate =
            std::stod(argv[3]);

        const std::string outputDir =
            (argc >= 5) ? argv[4] : "..";

        DataGenerator generator(42);

        generator.generateRS(
            numR,
            numS,
            matchRate,
            outputDir);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr
            << "Error: "
            << e.what()
            << '\n';

        return 1;
    }
}

// Old Generator only 1.0 matrat ecapped a
//
// namespace fs = std::filesystem;
//
// class DataGenerator {
// public:
//     struct Employee {
//         std::string name; // Quoted string attribute
//         int did;          // Integer Foreign Key to Department
//     };
//
//     struct Department {
//         int did;          // Integer Primary Key (1, 2, 3...)
//         std::string name; // Quoted string attribute
//     };
//
//     explicit DataGenerator(unsigned int seed = 42) : rng_(seed) {}
//
//     /**
//      * Generates Employees.txt and Department.txt.
//      * Default output directory is ".." (parent folder).
//      */
//     void generate(int numEmployees, int numDepts, double matchRate, const std::string& outputDir = "..") {
//         const std::vector<std::string> firstNames = {
//             "Alex", "Jordan", "Taylor", "Morgan", "Casey", "Riley", "Avery", "Dakota",
//             "Sam", "Reese", "Quinn", "Skyler", "Cameron", "Rowan", "Emerson", "Finley"
//         };
//
//         const std::vector<std::string> lastNames = {
//             "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller",
//             "Davis", "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez", "Wilson"
//         };
//
//         const std::vector<std::string> deptBaseNames = {
//             "Engineering", "Human Resources", "Data Systems", "Finance",
//             "Marketing", "Research", "Mathematics", "Physics", "Robotics"
//         };
//
//         // 1. Generate Department Tuples (DID: 1, 2, 3 ... numDepts)
//         std::vector<Department> departments;
//         departments.reserve(numDepts);
//
//         for (int i = 1; i <= numDepts; ++i) {
//             int did = i; // Clean integer Primary Key
//             std::string name = deptBaseNames[(i - 1) % deptBaseNames.size()] + " " + std::to_string(i);
//             departments.push_back({did, name});
//         }
//
//         // 2. Generate Employees Tuples
//         int matchingCount = static_cast<int>(std::round(numEmployees * matchRate));
//         matchingCount = std::min(numEmployees, std::max(0, matchingCount));
//
//         std::vector<Employee> employees;
//         employees.reserve(numEmployees);
//
//         std::uniform_int_distribution<int> validDeptDist(1, numDepts);
//         std::uniform_int_distribution<int> invalidDeptDist(numDepts + 1, numDepts + numEmployees + 1000);
//
//         for (int i = 1; i <= numEmployees; ++i) {
//             // Randomize Employee Name
//             std::string firstName = firstNames[rng_() % firstNames.size()];
//             std::string lastName = lastNames[rng_() % lastNames.size()];
//             std::string name = firstName + " " + lastName + " " + std::to_string(i);
//
//             int did;
//             if (i <= matchingCount) {
//                 // Match existing department integer ID [1 .. numDepts]
//                 did = validDeptDist(rng_);
//             } else {
//                 // Non-matching department integer ID [numDepts + 1 ...]
//                 did = invalidDeptDist(rng_);
//             }
//
//             employees.push_back({name, did});
//         }
//
//         // Shuffle employees to mix matching and non-matching rows
//         std::shuffle(employees.begin(), employees.end(), rng_);
//
//         // 3. Ensure parent output directory exists and write files
//         if (!outputDir.empty()) {
//             fs::create_directories(outputDir);
//         }
//
//         std::string pathEmp = (fs::path(outputDir) / "Employees.txt").string();
//         std::string pathDept = (fs::path(outputDir) / "Department.txt").string();
//
//         writeEmployees(pathEmp, employees);
//         writeDepartments(pathDept, departments);
//
//         std::cout << "[OK] DataGenerator created files in parent directory (" << outputDir << "):\n";
//         std::cout << "     - " << pathEmp << " (" << numEmployees << " tuples)\n";
//         std::cout << "     - " << pathDept << " (" << numDepts << " tuples, Match Rate: " << matchRate << ")\n";
//     }
//
// private:
//     std::mt19937 rng_;
//
//     void writeEmployees(const std::string& filepath, const std::vector<Employee>& tuples) {
//         std::ofstream outFile(filepath);
//         if (!outFile.is_open()) {
//             throw std::runtime_error("Failed to open file for writing: " + filepath);
//         }
//
//         outFile << "Employees (Name, DID) = {\n";
//         for (const auto& t : tuples) {
//             outFile << "  '" << t.name << "', " << t.did << "\n";
//         }
//         outFile << "}\n";
//     }
//
//     void writeDepartments(const std::string& filepath, const std::vector<Department>& tuples) {
//         std::ofstream outFile(filepath);
//         if (!outFile.is_open()) {
//             throw std::runtime_error("Failed to open file for writing: " + filepath);
//         }
//
//         outFile << "Department (DID, Name) = {\n";
//         for (const auto& t : tuples) {
//             outFile << "  " << t.did << ", '" << t.name << "'\n";
//         }
//         outFile << "}\n";
//     }
// };
//
// int main(int argc, char* argv[]) {
//     if (argc < 4) {
//         std::cout << "Usage: " << argv[0] << " <numEmployees> <numDepartments> <matchRate> [outputDirectory]\n";
//         std::cout << "Example (writes to parent directory '..'): " << argv[0] << " 1000 1000 1.0 ..\n";
//         return 1;
//     }
//
//     int numEmployees = std::stoi(argv[1]);
//     int numDepts = std::stoi(argv[2]);
//     double matchRate = std::stod(argv[3]);
//     // Defaults to ".." if 4th argument is omitted
//     std::string outputDir = (argc >= 5) ? argv[4] : "..";
//
//     try {
//         DataGenerator generator(42);
//         generator.generate(numEmployees, numDepts, matchRate, outputDir);
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         return 1;
//     }
//
//     return 0;
// }