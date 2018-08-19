#include "random.h"
#include <ctime>
#include <cstdlib>

namespace base
{
    namespace utils
    {
        using namespace std;

        Random::Random()
        {
        }

        Random::~Random()
        {
        }

        bool Random::Setup()
        {
            srand(time(NULL));
            return true;
        }

        std::string Random::GenRandomString(int size)
        {
            string str;
            static const char RANDOM_DICT[] = {
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
            };
            static int RANDOM_DICT_SIZE = sizeof(RANDOM_DICT) / sizeof(char);
            str.reserve(size);
            int r;
            for (int i = 0; i < size; i++) {
                r = rand() % RANDOM_DICT_SIZE;
                str.push_back(RANDOM_DICT[r]);
            }
            return str;
        }

        int Random::GenRandomNum(int max)
        {
            if (max == 0) {
                return 0;
            }
            return std::rand() % max;
        }

        int Random::GenRandomNum(int min, int max)
        {
            if (max == min) {
                return min;
            }
            return (std::rand() % (max - min)) + min;
        }

        float Random::GenRandom_0_1()
        {
            return std::rand() / (float)RAND_MAX;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
