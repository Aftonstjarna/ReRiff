#include "driving.hpp"
#include "native_common.hpp"
#include <cstring>

using namespace reriff;
static int checks = 0;
static void expect(bool value, const char *label) {
    ++checks;
    if (!value)
        throw std::runtime_error(label);
}
int main(int argc, char **argv) {
    try {
        drive::Params p;
        constexpr double rad = .017453293005625408;
        expect(std::abs(drive::steerLimit(p, 0) - 30 * rad) < 1e-6,
               "low-speed original steering limit");
        expect(std::abs(drive::steerLimit(p, float(100 * .44703999161720276)) - 6 * rad) < 1e-6,
               "100mph original steering limit");
        expect(drive::steerLimit(p, 80) == drive::steerLimit(p, -80),
               "steering uses absolute speed");
        expect(drive::brakeInput(false, 1, 20, -.5f) == .5f, "brake request");
        expect(drive::brakeInput(true, 1, -.1f, 1) == 0, "reverse threshold");
        expect(drive::brakeInput(true, 1, -.3f, 1) == 1, "reverse braking");
        rr::Json report = {{"checks", checks}, {"passed", true}};
        if (argc == 2) {
            rr::fs::path reference = argv[1];
            std::ifstream steering(reference / "original-steering.tsv"),
                brakes(reference / "original-braking.tsv");
            if (!steering || !brakes)
                throw std::runtime_error("original driving scalar capture missing");
            int steerCases = 0, brakeCases = 0;
            float speed, drift, input, bias, expected, maxError = 0;
            while (steering >> speed >> drift >> input >> bias >> expected) {
                float result = drive::steerLimit(p, speed, drift, input, bias);
                maxError = std::max(maxError, std::abs(result - expected));
                expect(result == expected, "original steering parity");
                steerCases++;
            }
            int toggle, gear;
            while (brakes >> toggle >> gear >> speed >> input >> expected) {
                expect(drive::brakeInput(toggle != 0, gear, speed, input) == expected,
                       "original brake parity");
                brakeCases++;
            }
            expect(steerCases == 108 && brakeCases == 90, "complete original vehicle scalar cases");
            report.update({{"checks", checks},
                           {"original_steering_cases", steerCases},
                           {"original_brake_cases", brakeCases},
                           {"max_steering_error", maxError}});
        }
        std::cout << report.dump(2) << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "driving test: " << e.what() << '\n';
        return 1;
    }
}
